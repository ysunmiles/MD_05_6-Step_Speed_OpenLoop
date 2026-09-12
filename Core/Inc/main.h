/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os.h"

#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern osThreadId_t DisplayTaskHandle;
extern osThreadId_t MotorCtrlTaskHandle;
extern osThreadId_t MonitorTaskHandle;

extern osMessageQueueId_t MotorDatasQueueHandle;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY0_Pin GPIO_PIN_2
#define KEY0_GPIO_Port GPIOE
#define KEY0_EXTI_IRQn EXTI2_IRQn
#define BEMF_W_Pin GPIO_PIN_7
#define BEMF_W_GPIO_Port GPIOF
#define BEMF_V_Pin GPIO_PIN_8
#define BEMF_V_GPIO_Port GPIOF
#define BEMF_U_Pin GPIO_PIN_9
#define BEMF_U_GPIO_Port GPIOF
#define CTRL_SD_Pin GPIO_PIN_10
#define CTRL_SD_GPIO_Port GPIOF
#define VTEMP_Pin GPIO_PIN_0
#define VTEMP_GPIO_Port GPIOA
#define AMP_IW_Pin GPIO_PIN_3
#define AMP_IW_GPIO_Port GPIOA
#define AMP_IV_Pin GPIO_PIN_6
#define AMP_IV_GPIO_Port GPIOA
#define AMP_IU_Pin GPIO_PIN_0
#define AMP_IU_GPIO_Port GPIOB
#define VBUS_Pin GPIO_PIN_1
#define VBUS_GPIO_Port GPIOB
#define OLED_GND_Pin GPIO_PIN_0
#define OLED_GND_GPIO_Port GPIOG
#define HALLU_Pin GPIO_PIN_10
#define HALLU_GPIO_Port GPIOH
#define HALLV_Pin GPIO_PIN_11
#define HALLV_GPIO_Port GPIOH
#define HALLW_Pin GPIO_PIN_12
#define HALLW_GPIO_Port GPIOH
#define PWM_UL_Pin GPIO_PIN_13
#define PWM_UL_GPIO_Port GPIOB
#define PWM_VL_Pin GPIO_PIN_14
#define PWM_VL_GPIO_Port GPIOB
#define PWM_WL_Pin GPIO_PIN_15
#define PWM_WL_GPIO_Port GPIOB
#define OLED_SCL_Pin GPIO_PIN_14
#define OLED_SCL_GPIO_Port GPIOD
#define OLED_SDA_Pin GPIO_PIN_0
#define OLED_SDA_GPIO_Port GPIOD
#define OLED_VCC_Pin GPIO_PIN_4
#define OLED_VCC_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
