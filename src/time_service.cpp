#include "time_service.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"
#include "rtc.h"

#define CLASS_NAME "TimeServi"
#include "log.h"

TimeService::TimeService() {}

void TimeService::setAlarmFor(uint8_t delta, TimeUnit timeUnit)
{
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    logInfo("Current Time:   %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
    if (timeUnit == SECONDS)
    {
        localTime.Minutes += delta / 60;
        localTime.Seconds += delta % 60;
    }
    else if (timeUnit == MINUTES)
    {
        localTime.Hours += delta / 60;
        localTime.Minutes += delta % 60;
    }
    else if (timeUnit == HOURS)
    {
        localTime.Hours += delta;
    }

    if (localTime.Seconds >= 60)
    {
        localTime.Seconds -= 60;
        localTime.Minutes += 1;
    }
    if (localTime.Minutes >= 60)
    {
        localTime.Minutes -= 60;
        localTime.Hours += 1;
    }
    if (localTime.Hours >= 24)
    {
        localTime.Hours -= 24;
    }
    HAL_RTC_SetLocalAlarm(localTime.Hours, localTime.Minutes, localTime.Seconds);
    RTC_TimeTypeDef localAlarm = HAL_RTC_GetLocalAlarm();
    
    logInfo("Set Alarm Time: %02d:%02d:%02d\r\n", localAlarm.Hours, localAlarm.Minutes, localAlarm.Seconds);
}

void TimeService::populateDataWithDateAndTime(uint8_t *data)
{
    RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();
    // date
    data[1] = localDate.Month;
    data[2] = localDate.Date;
    data[3] = localDate.Year;
    logInfo("Populate Data with Date: %02d.%02d.%02d\r\n", localDate.Date, localDate.Month, localDate.Year);

    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    // time
    data[4] = localTime.Hours;
    data[5] = localTime.Minutes;
    data[6] = localTime.Seconds;
    logInfo("Populate Data with Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
}

void TimeService::populateDataWithAlarmTimeFor(uint8_t *data, uint8_t delta, TimeUnit timeUnit)
{
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    if (timeUnit == SECONDS)
    {
        localTime.Minutes += delta / 60;
        localTime.Seconds += delta % 60;
    }
    else if (timeUnit == MINUTES)
    {
        localTime.Hours += delta / 60;
        localTime.Minutes += delta % 60;
    }
    else if (timeUnit == HOURS)
    {
        localTime.Hours += delta;
    }

    if (localTime.Seconds >= 60)
    {
        localTime.Seconds -= 60;
        localTime.Minutes += 1;
    }
    if (localTime.Minutes >= 60)
    {
        localTime.Minutes -= 60;
        localTime.Hours += 1;
    }
    if (localTime.Hours >= 24)
    {
        localTime.Hours -= 24;
    }
    data[7] = localTime.Hours;
    data[8] = localTime.Minutes;
    data[9] = localTime.Seconds;
    logInfo("Populate Data with Alarm Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
}

void TimeService::setDateAndTime(uint8_t *data)
{
    // 1:3 bytes contains current date
    // 4:6 bytes contains current time
    HAL_RTC_SetLocalDate(data[1], data[2], data[3]);
    RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();
    logInfo("Set Date: %02d.%02d.%02d\r\n", localDate.Date, localDate.Month, localDate.Year);

    HAL_RTC_SetLocalTime(data[4], data[5], data[6]);
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    logInfo("Set Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
}

void TimeService::setAlarmTime(uint8_t *data)
{
    // 7:9 bytes contains alarm time
    HAL_RTC_SetLocalAlarm(data[7], data[8], data[9]);
    RTC_TimeTypeDef localAlarm = HAL_RTC_GetLocalAlarm();
    logInfo("Set Alarm Time: %02d:%02d:%02d\r\n", localAlarm.Hours, localAlarm.Minutes, localAlarm.Seconds);
}
