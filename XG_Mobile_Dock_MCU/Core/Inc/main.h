/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LOCK_SW_Pin GPIO_PIN_13
#define LOCK_SW_GPIO_Port GPIOC
#define LOCK_SW_EXTI_IRQn EXTI4_15_IRQn
#define LED_WHITE_Pin GPIO_PIN_14
#define LED_WHITE_GPIO_Port GPIOC
#define LED_RED_Pin GPIO_PIN_15
#define LED_RED_GPIO_Port GPIOC
#define LOCK_DET_Pin GPIO_PIN_0
#define LOCK_DET_GPIO_Port GPIOF
#define RSV0_Pin GPIO_PIN_1
#define RSV0_GPIO_Port GPIOF
#define CON_DET_Pin GPIO_PIN_0
#define CON_DET_GPIO_Port GPIOA
#define CON_DET_EXTI_IRQn EXTI0_1_IRQn
#define PWREN_Pin GPIO_PIN_1
#define PWREN_GPIO_Port GPIOA
#define PWREN_EXTI_IRQn EXTI0_1_IRQn
#define RST_Pin GPIO_PIN_2
#define RST_GPIO_Port GPIOA
#define RST_EXTI_IRQn EXTI2_3_IRQn
#define PWROK_Pin GPIO_PIN_3
#define PWROK_GPIO_Port GPIOA
#define AC_LOSS_Pin GPIO_PIN_4
#define AC_LOSS_GPIO_Port GPIOA
#define MCU_IRQ_Pin GPIO_PIN_5
#define MCU_IRQ_GPIO_Port GPIOA
#define SYS_ON_Pin GPIO_PIN_6
#define SYS_ON_GPIO_Port GPIOA
#define PWR_SW_Pin GPIO_PIN_7
#define PWR_SW_GPIO_Port GPIOA
#define SYS_DET_Pin GPIO_PIN_0
#define SYS_DET_GPIO_Port GPIOB
#define PCI_12V_EN_Pin GPIO_PIN_15
#define PCI_12V_EN_GPIO_Port GPIOA
#define WAKE_Pin GPIO_PIN_4
#define WAKE_GPIO_Port GPIOB
#define PERST_Pin GPIO_PIN_5
#define PERST_GPIO_Port GPIOB
#define CLKREQ_Pin GPIO_PIN_6
#define CLKREQ_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
