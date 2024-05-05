#include "main.h"
#include <stdio.h>

extern SMBUS_HandleTypeDef hsmbus1;
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;

typedef enum {
    MCU_RESET=0,
    CABLE_DETECT,
    CABLE_LOCK,
    POWER_OFF,
    POWER_ON,
    PCIE_RESET,
    CONNECTED,
} fsm_state_t;

typedef enum {
    NONE,
    WHITE,
    RED,
} led_colour_t;

const char * const gFSMStateStrings[] = {
    [MCU_RESET] = "MCU_RESET",
    [CABLE_DETECT] = "CABLE_DETECT",
    [CABLE_LOCK] = "CABLE_LOCK",
    [POWER_OFF] = "POWER_OFF",
    [POWER_ON] = "POWER_ON",
    [PCIE_RESET] = "PCIE_RESET",
    [CONNECTED] = "CONNECTED",
};

typedef struct {
    fsm_state_t fsm;
    int lock_switch;
    int connector_detect;
    int power_enable;
    int host_reset;
} state_t;

state_t gState;

int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        case LOCK_SW_Pin: {
            gState.lock_switch = HAL_GPIO_ReadPin(LOCK_SW_GPIO_Port, LOCK_SW_Pin) == GPIO_PIN_RESET;
            printf("Pin changed: LOCK_SW = %d\n", gState.lock_switch);
            break;
        }
        case CON_DET_Pin: {
            gState.connector_detect = HAL_GPIO_ReadPin(CON_DET_GPIO_Port, CON_DET_Pin) == GPIO_PIN_RESET;
            printf("Pin changed: CON_DET = %d\n", gState.connector_detect);
            break;
        }
        case PWREN_Pin: {
            gState.power_enable = HAL_GPIO_ReadPin(PWREN_GPIO_Port, PWREN_Pin) == GPIO_PIN_RESET;
            printf("Pin changed: PWREN = %d\n", gState.power_enable);
            break;
        }
        case RST_Pin: {
            gState.host_reset = HAL_GPIO_ReadPin(RST_GPIO_Port, RST_Pin) == GPIO_PIN_RESET;
            printf("Pin changed: RST = %d\n", gState.host_reset);
            break;
        }
        default: {
            printf("Unknown pin changed = %d\n", GPIO_Pin);
            break;
        }
    }
}

void init_gpio_state(state_t *state) {
    state->lock_switch = HAL_GPIO_ReadPin(LOCK_SW_GPIO_Port, LOCK_SW_Pin) == GPIO_PIN_RESET;
    state->connector_detect = HAL_GPIO_ReadPin(CON_DET_GPIO_Port, CON_DET_Pin) == GPIO_PIN_RESET;
    state->power_enable = HAL_GPIO_ReadPin(PWREN_GPIO_Port, PWREN_Pin) == GPIO_PIN_RESET;
    state->host_reset = HAL_GPIO_ReadPin(RST_GPIO_Port, RST_Pin) == GPIO_PIN_RESET;
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

void pcie_reset(int enable) {
    printf("PCIe reset = %d\n", enable);
    if (enable) {
        HAL_GPIO_WritePin(PERST_GPIO_Port, PERST_Pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(PERST_GPIO_Port, PERST_Pin, GPIO_PIN_SET);
    }
}

void toggle_external_board(void) {
    HAL_GPIO_WritePin(PWR_SW_GPIO_Port, PWR_SW_Pin, GPIO_PIN_RESET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(PWR_SW_GPIO_Port, PWR_SW_Pin, GPIO_PIN_SET);
}

void turn_power_on() {
    printf("Turning on PCIe power\n");
    HAL_GPIO_WritePin(PCI_12V_EN_GPIO_Port, PCI_12V_EN_Pin, GPIO_PIN_SET);
    toggle_external_board();
    HAL_GPIO_WritePin(PWROK_GPIO_Port, PWROK_Pin, GPIO_PIN_SET);
}

void turn_power_off() {
    printf("Turning off PCIe power\n");
    HAL_GPIO_WritePin(PWROK_GPIO_Port, PWROK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PCI_12V_EN_GPIO_Port, PCI_12V_EN_Pin, GPIO_PIN_RESET);
    toggle_external_board();
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
    // PCIe reset
    if (next == PCIE_RESET) {
        pcie_reset(1);
    } else if (prev == PCIE_RESET) {
        pcie_reset(0);
    }
    // power off/on
    if (prev > POWER_OFF && next <= POWER_OFF) {
        turn_power_off();
    } else if (prev < POWER_ON && next >= POWER_ON) {
        turn_power_on();
    }
}

void main_fsm_iteration(void) {
    printf("Enter main FSM with state = %s\n", gFSMStateStrings[gState.fsm]);
    switch (gState.fsm) {
        case MCU_RESET: {
            init_gpio_state(&gState);
            transition_state(&gState, CABLE_DETECT);
            break;
        }
        case CABLE_DETECT: {
            if (gState.connector_detect) {
                transition_state(&gState, CABLE_LOCK);
            } else {
                __WFI();
            }
            break;
        }
        case CABLE_LOCK: {
            if (!gState.connector_detect) {
                transition_state(&gState, CABLE_DETECT);
            } else if (gState.lock_switch) {
                transition_state(&gState, POWER_OFF);
            } else {
                __WFI();
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
            } else {
                __WFI();
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
            } else if (gState.host_reset) {
                transition_state(&gState, PCIE_RESET);
            } else {
                __WFI();
            }
            break;
        }
        case PCIE_RESET: {
            if (!gState.connector_detect) {
                transition_state(&gState, CABLE_DETECT);
            } else if (!gState.lock_switch) {
                transition_state(&gState, CABLE_LOCK);
            } else if (!gState.power_enable) {
                transition_state(&gState, POWER_OFF);
            } else if (!gState.host_reset) {
                transition_state(&gState, CONNECTED);
            } else {
                __WFI();
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
            } else if (gState.host_reset) {
                transition_state(&gState, PCIE_RESET);
            } else {
                __WFI();
            }
            break;
        }
        default: {
            printf("Unknown state = %d\n", gState.fsm);
            break;
        }
    }
}
