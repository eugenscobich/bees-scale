/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

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
#define RED_LED_Pin GPIO_PIN_13
#define RED_LED_GPIO_Port GPIOC
#define BATERY_LEVEL_Pin GPIO_PIN_1
#define BATERY_LEVEL_GPIO_Port GPIOA
#define NRF_CSN_Pin GPIO_PIN_4
#define NRF_CSN_GPIO_Port GPIOA
#define NRF_CE_Pin GPIO_PIN_0
#define NRF_CE_GPIO_Port GPIOB
#define FOTO_RESISTOR_Pin GPIO_PIN_1
#define FOTO_RESISTOR_GPIO_Port GPIOB
#define BOOT1_Pin GPIO_PIN_2
#define BOOT1_GPIO_Port GPIOB
#define HX711_DT_1_Pin GPIO_PIN_12
#define HX711_DT_1_GPIO_Port GPIOB
#define HX711_SCK_Pin GPIO_PIN_13
#define HX711_SCK_GPIO_Port GPIOB
#define HX711_DT_2_Pin GPIO_PIN_14
#define HX711_DT_2_GPIO_Port GPIOB
#define HX711_DT_3_Pin GPIO_PIN_15
#define HX711_DT_3_GPIO_Port GPIOB
#define SIM800C_PWR_Pin GPIO_PIN_8
#define SIM800C_PWR_GPIO_Port GPIOA
#define SIM800C_DTR_Pin GPIO_PIN_15
#define SIM800C_DTR_GPIO_Port GPIOA
#define HTU21D_1_Pin GPIO_PIN_3
#define HTU21D_1_GPIO_Port GPIOB
#define HTU21D_2_Pin GPIO_PIN_4
#define HTU21D_2_GPIO_Port GPIOB
#define HTU21D_3_Pin GPIO_PIN_5
#define HTU21D_3_GPIO_Port GPIOB
#define GREEN_LED_Pin GPIO_PIN_6
#define GREEN_LED_GPIO_Port GPIOB
#define BUTTON_Pin GPIO_PIN_7
#define BUTTON_GPIO_Port GPIOB
#define NRF_IRQ_Pin GPIO_PIN_8
#define NRF_IRQ_GPIO_Port GPIOB
#define BATERY_LEVEL_CHECK_Pin GPIO_PIN_9
#define BATERY_LEVEL_CHECK_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
