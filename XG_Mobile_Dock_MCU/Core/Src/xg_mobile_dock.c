#include "main.h"
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim1;

typedef enum {
    MCU_RESET=0,
    DEVICE_IDLE,
    CABLE_DETECT,
    CABLE_LOCK,
    POWER_OFF,
    POWER_ON,
} fsm_state_t;

typedef enum {
    I2C_STATE_IDLE,
    I2C_STATE_ACCESS_REG,
    I2C_STATE_ACCESS_SIZE,
    I2C_STATE_WAIT_DATA,
    I2C_STATE_SEND_DATA,
    I2C_STATE_WAIT_ACK,
} i2c_state_t;

typedef enum {
    NONE,
    WHITE,
    RED,
} led_colour_t;

static const char * const gFSMStateStrings[] = {
    [MCU_RESET] = "MCU_RESET",
    [DEVICE_IDLE] = "DEVICE_IDLE",
    [CABLE_DETECT] = "CABLE_DETECT",
    [CABLE_LOCK] = "CABLE_LOCK",
    [POWER_OFF] = "POWER_OFF",
    [POWER_ON] = "POWER_ON",
};

typedef struct {
    fsm_state_t fsm;
    int debounce_pins;
    int lock_switch;
    int connector_detect;
    int power_enable;
    int external_board_detected;
    i2c_state_t i2c;
    unsigned char i2cBuffer[256];
    int i2cCmd;
    int i2cReg;
    int i2cSize;
    unsigned char i2cRegData1;
} state_t;

static state_t gState;

int __io_putchar(int ch) {
    if (ch == '\n') {
        uint8_t r = '\r';
        HAL_UART_Transmit(&huart1, &r, 1, 0xFFFF);
    }
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == RST_Pin) {
        GPIO_PinState reset = HAL_GPIO_ReadPin(RST_GPIO_Port, RST_Pin);
        // passthrough directly to PERST#
        HAL_GPIO_WritePin(PERST_GPIO_Port, PERST_Pin, reset);
        printf("Pin changed: RST = %d\n", reset == GPIO_PIN_RESET);
    } else if (GPIO_Pin == PWREN_Pin) {
        gState.power_enable = HAL_GPIO_ReadPin(PWREN_GPIO_Port, PWREN_Pin) == GPIO_PIN_RESET;
        printf("Pin changed: PWREN = %d\n", gState.power_enable);
    } else {
        if (gState.debounce_pins) {
            // reset timer, wait for last bounce
            HAL_TIM_Base_Stop_IT(&htim1);
        }
        HAL_TIM_Base_Start_IT(&htim1);
        gState.debounce_pins |= GPIO_Pin;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if ((gState.debounce_pins & LOCK_SW_Pin) != 0) {
        gState.lock_switch = HAL_GPIO_ReadPin(LOCK_SW_GPIO_Port, LOCK_SW_Pin) == GPIO_PIN_SET;
        printf("Pin changed: LOCK_SW = %d\n", gState.lock_switch);
    }
    if ((gState.debounce_pins & CON_DET_Pin) != 0) {
        gState.connector_detect = HAL_GPIO_ReadPin(CON_DET_GPIO_Port, CON_DET_Pin) == GPIO_PIN_RESET;
        printf("Pin changed: CON_DET = %d\n", gState.connector_detect);
    }
    gState.debounce_pins = 0;
    HAL_TIM_Base_Stop_IT(htim);
}

int is_external_board_on(void) {
    return HAL_GPIO_ReadPin(SYS_ON_GPIO_Port, SYS_ON_Pin) == GPIO_PIN_SET;
}

void init_gpio_state(state_t *state) {
    state->lock_switch = HAL_GPIO_ReadPin(LOCK_SW_GPIO_Port, LOCK_SW_Pin) == GPIO_PIN_SET;
    printf("LOCK_SW = %d, ", state->lock_switch);
    state->connector_detect = HAL_GPIO_ReadPin(CON_DET_GPIO_Port, CON_DET_Pin) == GPIO_PIN_RESET;
    printf("CON_DET = %d, ", state->connector_detect);
    state->power_enable = HAL_GPIO_ReadPin(PWREN_GPIO_Port, PWREN_Pin) == GPIO_PIN_RESET;
    printf("PWREN = %d, ", state->power_enable);
    GPIO_PinState reset = HAL_GPIO_ReadPin(RST_GPIO_Port, RST_Pin);
    printf("RST = %d, ", reset == GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PERST_GPIO_Port, PERST_Pin, reset);
    state->external_board_detected = HAL_GPIO_ReadPin(SYS_DET_GPIO_Port, SYS_DET_Pin) == GPIO_PIN_RESET;
    printf("SYS_DET = %d, ", state->external_board_detected);
    printf("SYS_ON = %d\n", is_external_board_on());
}

void update_cable_led(led_colour_t colour) {
    switch (colour) {
        case WHITE: {
            printf("Turn on white LED\n");
            HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_WHITE_GPIO_Port, LED_WHITE_Pin, GPIO_PIN_RESET);
            break;
        }
        case RED: {
            printf("Turn on red LED\n");
            HAL_GPIO_WritePin(LED_WHITE_GPIO_Port, LED_WHITE_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
            break;
        }
        case NONE:
        default: {
            printf("Turn off cable LED\n");
            HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_WHITE_GPIO_Port, LED_WHITE_Pin, GPIO_PIN_SET);
            break;
        }
    }
}

void toggle_external_board(int on) {
    int tries = 5;

    if (!gState.external_board_detected) {
        return;
    }
    printf("Toggling external power switch.");
    while (is_external_board_on() != on && tries-- > 0) {
        printf(".");
        HAL_Delay(1000);
        HAL_GPIO_WritePin(PWR_SW_GPIO_Port, PWR_SW_Pin, GPIO_PIN_RESET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(PWR_SW_GPIO_Port, PWR_SW_Pin, GPIO_PIN_SET);
    }
    if (tries >= 0) {
        printf("Done\n");
    } else {
        printf("Failed\n");
    }
}

void turn_power_on() {
    printf("Turning on PCIe power\n");
    HAL_GPIO_WritePin(PCI_12V_EN_GPIO_Port, PCI_12V_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PWROK_GPIO_Port, PWROK_Pin, GPIO_PIN_SET);
}

void turn_power_off() {
    printf("Turning off PCIe power\n");
    HAL_GPIO_WritePin(PWROK_GPIO_Port, PWROK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PCI_12V_EN_GPIO_Port, PCI_12V_EN_Pin, GPIO_PIN_RESET);
}

void assert_ec_irq() {
    printf("Asserting EC IRQ\n");
    HAL_GPIO_WritePin(MCU_IRQ_GPIO_Port, MCU_IRQ_Pin, GPIO_PIN_RESET);
}

void clear_ec_irq() {
    printf("Clearing EC IRQ\n");
    HAL_GPIO_WritePin(MCU_IRQ_GPIO_Port, MCU_IRQ_Pin, GPIO_PIN_SET);
}

void assert_lock_det() {
    printf("Asserting LOCK_DET\n");
    HAL_GPIO_WritePin(LOCK_DET_GPIO_Port, LOCK_DET_Pin, GPIO_PIN_SET);
}

void clear_lock_det() {
    printf("Clearing LOCK_DET\n");
    HAL_GPIO_WritePin(LOCK_DET_GPIO_Port, LOCK_DET_Pin, GPIO_PIN_RESET);
}

void transition_state(state_t *state, fsm_state_t next) {
    fsm_state_t prev = state->fsm;

    printf("Transition %s -> %s\n", gFSMStateStrings[prev], gFSMStateStrings[next]);
    // update cable LED
    if (next == CABLE_LOCK) {
        update_cable_led(WHITE);
    } else if (next == POWER_ON && prev < next) {
        update_cable_led(RED);
    } else if (next < CABLE_LOCK && prev > next) {
        update_cable_led(NONE);
    }
    // LOCK_DET
    if (next > CABLE_LOCK && prev < next) {
        assert_lock_det();
    } else if (next <= CABLE_LOCK && prev > next) {
        clear_lock_det();
    }
    // external power
    if (next == POWER_OFF && prev < next) {
        toggle_external_board(1);
    } else if (next <= CABLE_DETECT && prev > next) {
        toggle_external_board(0);
    }
    // power off/on
    if (prev > POWER_OFF && next <= POWER_OFF) {
        turn_power_off();
    } else if (prev < POWER_ON && next >= POWER_ON) {
        turn_power_on();
    }
    // send IRQ
    if (prev < CABLE_LOCK && next >= CABLE_LOCK) {
        assert_ec_irq();
    }
    // reset I2C state when unplugged
    if (prev == MCU_RESET || (prev > CABLE_DETECT && next <= CABLE_DETECT)) {
        state->i2cRegData1 = 0xff;
    }
    state->fsm = next;
}

void main_fsm_iteration(void) {
    fsm_state_t prev = gState.fsm;

    //printf("Enter main FSM with state = %s\n", gFSMStateStrings[gState.fsm]);
    switch (prev) {
        case MCU_RESET: {
            init_gpio_state(&gState);
            toggle_external_board(0);
            transition_state(&gState, DEVICE_IDLE);
            HAL_I2C_EnableListen_IT(&hi2c1);
            break;
        }
        case DEVICE_IDLE: {
            if (gState.i2cRegData1 == 0) {
                transition_state(&gState, CABLE_DETECT);
            }
            break;
        }
        case CABLE_DETECT: {
            if (gState.i2cRegData1 != 0) {
                transition_state(&gState, DEVICE_IDLE);
            } else if (gState.connector_detect) {
                transition_state(&gState, CABLE_LOCK);
            }
            break;
        }
        case CABLE_LOCK: {
            if (gState.i2cRegData1 != 0) {
                transition_state(&gState, DEVICE_IDLE);
            } else if (!gState.connector_detect) {
                transition_state(&gState, CABLE_DETECT);
            } else if (gState.lock_switch) {
                transition_state(&gState, POWER_OFF);
            }
            break;
        }
        case POWER_OFF: {
            if (gState.i2cRegData1 != 0) {
                transition_state(&gState, DEVICE_IDLE);
            } else if (!gState.connector_detect) {
                transition_state(&gState, CABLE_DETECT);
            } else if (!gState.lock_switch) {
                transition_state(&gState, CABLE_LOCK);
            } else if (gState.power_enable) {
                transition_state(&gState, POWER_ON);
            }
            break;
        }
        case POWER_ON: {
            if (gState.i2cRegData1 != 0) {
                transition_state(&gState, DEVICE_IDLE);
            } else if (!gState.connector_detect) {
                transition_state(&gState, CABLE_DETECT);
            } else if (!gState.lock_switch) {
                transition_state(&gState, CABLE_LOCK);
            } else if (!gState.power_enable) {
                transition_state(&gState, POWER_OFF);
            }
            break;
        }
        default: {
            printf("Unknown state = %d\n", gState.fsm);
            break;
        }
    }
    if (gState.fsm == prev) {
        __WFI();
    }
}

/* I2C Handling */

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
	if(TransferDirection == I2C_DIRECTION_TRANSMIT)  // if the master wants to transmit the data
	{
        //printf("Got I2C transmit!\n");
        gState.i2c = I2C_STATE_IDLE;
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, gState.i2cBuffer, 1, I2C_FIRST_AND_LAST_FRAME);
	}
	else
	{
        if (gState.i2c == I2C_STATE_SEND_DATA) {
            gState.i2c = I2C_STATE_WAIT_ACK;
            HAL_I2C_Slave_Seq_Transmit_IT(hi2c, gState.i2cBuffer, gState.i2cSize, I2C_FIRST_AND_LAST_FRAME);
        } else {
            gState.i2c = I2C_STATE_IDLE;
            printf("I2C: Unexpected read!\n");
            HAL_I2C_Slave_Seq_Transmit_IT(hi2c, NULL, 0, I2C_FIRST_AND_LAST_FRAME);
        }
	}
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (gState.i2c == I2C_STATE_IDLE) {
        gState.i2cCmd = gState.i2cBuffer[0];
        if (gState.i2cCmd == 0xA0) {
            gState.i2c = I2C_STATE_ACCESS_REG;
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, gState.i2cBuffer, 1, I2C_NEXT_FRAME);
        } else if (gState.i2cCmd == 0xA1 || gState.i2cCmd == 0xA2 || gState.i2cCmd == 0xA3) {
            gState.i2cReg = 0;
            if (gState.i2cCmd == 0xA1) {
                clear_ec_irq();
                gState.i2cBuffer[0] = 0xDC;
                gState.i2cSize = 1;
            } else if (gState.i2cCmd == 0xA2) {
                gState.i2cBuffer[0] = gState.i2cRegData1 == 0 ? 2 : 1;
                gState.i2cSize = 1;
            } else if (gState.i2cCmd == 0xA3) {
                gState.i2cBuffer[0] = 1;
                gState.i2cSize = 1;
            }
            gState.i2c = I2C_STATE_SEND_DATA;
        } else if (gState.i2cCmd == 0xE6) {
            gState.i2c = I2C_STATE_IDLE;
            // ignore this, we might get spammed it
        } else {
            gState.i2c = I2C_STATE_IDLE;
            printf("I2C: Unsupported CMD = 0x%02X\n", gState.i2cCmd);
        }
    } else if (gState.i2c == I2C_STATE_ACCESS_REG) {
        gState.i2cReg = gState.i2cBuffer[0];
        gState.i2c = I2C_STATE_ACCESS_SIZE;
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, gState.i2cBuffer, 1, I2C_NEXT_FRAME);
    } else if (gState.i2c == I2C_STATE_ACCESS_SIZE) {
        gState.i2cSize = gState.i2cBuffer[0];
        gState.i2c = I2C_STATE_WAIT_DATA;
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, gState.i2cBuffer, gState.i2cSize, I2C_LAST_FRAME);
    } else if (gState.i2c == I2C_STATE_WAIT_DATA) {
        printf("I2C: CMD = 0x%02X, reg = 0x%02X, size = %d\n", gState.i2cCmd, gState.i2cReg, gState.i2cSize);
        printf("I2C: Data = ");
        for (int i = 0; i < gState.i2cSize; i++) {
            printf("0x%02X ", gState.i2cBuffer[i]);
        }
        printf("\n");
        if (gState.i2cCmd == 0xA0 && gState.i2cSize == 1 && gState.i2cReg == 1) {
            gState.i2cRegData1 = gState.i2cBuffer[0];
        }
        gState.i2c = I2C_STATE_IDLE;
    }
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (gState.i2c == I2C_STATE_WAIT_ACK) {
        if (gState.i2cCmd != 0xA3) { // too verbose
            printf("I2C: CMD = 0x%02X, reg = 0x%02X, size = %d\n", gState.i2cCmd, gState.i2cReg, gState.i2cSize);
            printf("I2C: Sent data = ");
            for (int i = 0; i < gState.i2cSize; i++) {
                printf("0x%02X ", gState.i2cBuffer[i]);
            }
            printf("\n");
        }
        gState.i2c = I2C_STATE_IDLE;
    } else {
        printf("I2C: Unexpected TX complete callback!\n");
        gState.i2c = I2C_STATE_IDLE;
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    printf("I2C: Bus error seen!\n");
    gState.i2c = I2C_STATE_IDLE;
}
