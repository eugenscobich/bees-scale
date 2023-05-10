/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 0x1;
  DateToUpdate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

RTC_TimeTypeDef HAL_RTC_GetLocalTime() {
  RTC_TimeTypeDef sTime = {0};
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  return sTime;
}

RTC_DateTypeDef HAL_RTC_GetLocalDate() {
  RTC_DateTypeDef sDate = {0};
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
  return sDate;
}

void HAL_RTC_SetLocalTime(uint8_t Hours, uint8_t Minutes, uint8_t Seconds) {
  RTC_TimeTypeDef sTime = {0};
  sTime.Hours = Hours;
  sTime.Minutes = Minutes;
  sTime.Seconds = Seconds;
  HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}

void HAL_RTC_SetLocalDate(uint8_t Month, uint8_t Date, uint8_t Year) {
  RTC_DateTypeDef sDate = {0};
  sDate.Month = Month;
  sDate.Date = Date;
  sDate.Year = Year;
  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void HAL_RTC_SetLocalAlarm(uint8_t Hours, uint8_t Minutes, uint8_t Seconds) {
  RTC_AlarmTypeDef sAlarm = {0};
  sAlarm.AlarmTime.Hours = Hours;
  sAlarm.AlarmTime.Minutes = Minutes;
  sAlarm.AlarmTime.Seconds = Seconds;

  sAlarm.Alarm = RTC_ALARM_A;
  HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BIN);
}

RTC_TimeTypeDef HAL_RTC_GetLocalAlarm() {
  RTC_AlarmTypeDef sAlarm = {0};
  sAlarm.Alarm = RTC_ALARM_A;
  HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN);
  return sAlarm.AlarmTime;
}


/* USER CODE END 1 */
