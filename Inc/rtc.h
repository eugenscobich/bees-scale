/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.h
  * @brief   This file contains all the function prototypes for
  *          the rtc.c file
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
#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_RTC_Init(void);

/* USER CODE BEGIN Prototypes */

RTC_TimeTypeDef HAL_RTC_GetLocalTime();
RTC_DateTypeDef HAL_RTC_GetLocalDate();
void HAL_RTC_SetLocalTime(uint8_t Hours, uint8_t Minutes, uint8_t Seconds);
void HAL_RTC_SetLocalDate(uint8_t Month, uint8_t Date, uint8_t Year);
void HAL_RTC_SetLocalAlarm(uint8_t Hours, uint8_t Minutes, uint8_t Seconds);
RTC_TimeTypeDef HAL_RTC_GetLocalAlarm();

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H__ */

