#include "main.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim1;

typedef enum {
    MCU_RESET=0,
    CABLE_DETECT,
    CABLE_LOCK,
    POWER_OFF,
    POWER_ON,
    CONNECTED,
} fsm_state_t;

typedef enum {
    NONE,
    WHITE,
    RED,
} led_colour_t;

static const char * const gFSMStateStrings[] = {
    [MCU_RESET] = "MCU_RESET",
    [CABLE_DETECT] = "CABLE_DETECT",
    [CABLE_LOCK] = "CABLE_LOCK",
    [POWER_OFF] = "POWER_OFF",
    [POWER_ON] = "POWER_ON",
    [CONNECTED] = "CONNECTED",
};

typedef struct {
    fsm_state_t fsm;
    int debounce_pins;
    int lock_switch;
    int connector_detect;
    int power_enable;
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

void init_gpio_state(state_t *state) {
    state->lock_switch = HAL_GPIO_ReadPin(LOCK_SW_GPIO_Port, LOCK_SW_Pin) == GPIO_PIN_SET;
    printf("LOCK_SW = %d, ", gState.lock_switch);
    state->connector_detect = HAL_GPIO_ReadPin(CON_DET_GPIO_Port, CON_DET_Pin) == GPIO_PIN_RESET;
    printf("CON_DET = %d, ", gState.connector_detect);
    state->power_enable = HAL_GPIO_ReadPin(PWREN_GPIO_Port, PWREN_Pin) == GPIO_PIN_RESET;
    printf("PWREN = %d, ", gState.power_enable);
    GPIO_PinState reset = HAL_GPIO_ReadPin(RST_GPIO_Port, RST_Pin);
    printf("RST = %d, ", reset == GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PERST_GPIO_Port, PERST_Pin, reset);
    printf("SYS_ON = %d\n", HAL_GPIO_ReadPin(SYS_ON_GPIO_Port, SYS_ON_Pin) == GPIO_PIN_SET);
}

void update_cable_led(led_colour_t colour) {
    switch (colour) {
        case WHITE: {
            printf("Turn on white LED\n");
            HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_WHITE_GPIO_Port, LED_WHITE_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LOCK_DET_GPIO_Port, LOCK_DET_Pin, GPIO_PIN_RESET);
            break;
        }
        case RED: {
            printf("Turn on red LED\n");
            HAL_GPIO_WritePin(LED_WHITE_GPIO_Port, LED_WHITE_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LOCK_DET_GPIO_Port, LOCK_DET_Pin, GPIO_PIN_SET);
            break;
        }
        case NONE:
        default: {
            printf("Turn off cable LED\n");
            HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_WHITE_GPIO_Port, LED_WHITE_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LOCK_DET_GPIO_Port, LOCK_DET_Pin, GPIO_PIN_RESET);
            break;
        }
    }
}

void toggle_external_board(void) {
    HAL_GPIO_WritePin(PWR_SW_GPIO_Port, PWR_SW_Pin, GPIO_PIN_RESET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(PWR_SW_GPIO_Port, PWR_SW_Pin, GPIO_PIN_SET);
}

void turn_power_on() {
    printf("Turning on PCIe power\n");
    if (HAL_GPIO_ReadPin(SYS_ON_GPIO_Port, SYS_ON_Pin) == GPIO_PIN_SET) {
        printf("External board already on.\n");
    } else {
        toggle_external_board();
    }
    HAL_GPIO_WritePin(PCI_12V_EN_GPIO_Port, PCI_12V_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PWROK_GPIO_Port, PWROK_Pin, GPIO_PIN_SET);
}

void turn_power_off() {
    printf("Turning off PCIe power\n");
    HAL_GPIO_WritePin(PWROK_GPIO_Port, PWROK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PCI_12V_EN_GPIO_Port, PCI_12V_EN_Pin, GPIO_PIN_RESET);
    if (HAL_GPIO_ReadPin(SYS_ON_GPIO_Port, SYS_ON_Pin) == GPIO_PIN_SET) {
        toggle_external_board();
    } else {
        printf("External board already off.\n");
    }
}

void transition_state(state_t *state, fsm_state_t next) {
    fsm_state_t prev = state->fsm;

    printf("Transition %s -> %s\n", gFSMStateStrings[prev], gFSMStateStrings[next]);
    // update cable LED
    if (next == CABLE_LOCK) {
        update_cable_led(WHITE);
    } else if (next == POWER_OFF && prev < next) {
        update_cable_led(RED);
    } else if (next <= CABLE_DETECT && prev > next) {
        update_cable_led(NONE);
    }
    // power off/on
    if (prev > POWER_OFF && next <= POWER_OFF) {
        turn_power_off();
    } else if (prev < POWER_ON && next >= POWER_ON) {
        turn_power_on();
    }
    state->fsm = next;
}

void main_fsm_iteration(void) {
    fsm_state_t prev = gState.fsm;

    //printf("Enter main FSM with state = %s\n", gFSMStateStrings[gState.fsm]);
    switch (prev) {
        case MCU_RESET: {
            init_gpio_state(&gState);
            transition_state(&gState, CABLE_DETECT);
            break;
        }
        case CABLE_DETECT: {
            if (gState.connector_detect) {
                transition_state(&gState, CABLE_LOCK);
            }
            break;
        }
        case CABLE_LOCK: {
            if (!gState.connector_detect) {
                transition_state(&gState, CABLE_DETECT);
            } else if (gState.lock_switch) {
                transition_state(&gState, POWER_OFF);
            }
            break;
        }
        case POWER_OFF: {
            if (!gState.connector_detect) {
                transition_state(&gState, CABLE_DETECT);
            } else if (!gState.lock_switch) {
                transition_state(&gState, CABLE_LOCK);
            } else if (gState.power_enable) {
                transition_state(&gState, POWER_ON);
            }
            break;
        }
        case POWER_ON: {
            if (!gState.connector_detect) {
                transition_state(&gState, CABLE_DETECT);
            } else if (!gState.lock_switch) {
                transition_state(&gState, CABLE_LOCK);
            } else if (!gState.power_enable) {
                transition_state(&gState, POWER_OFF);
            } else {
                transition_state(&gState, CONNECTED);
            }
            break;
        }
        case CONNECTED: {
            if (!gState.connector_detect) {
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
