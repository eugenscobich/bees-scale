#include "sensors_service.h"
#include "rtc.h"
#include <stdio.h>

#define CLASS_NAME "SensorSer"
#include "log.h"

#define NUMBER_OF_KNOWN_CALIBRATIONS 1

SensorsService::SensorsService(HX711 *hx711_1, HX711 *hx711_2, HX711 *hx711_3, DS18B20 *ds18b20_1, DS18B20 *ds18b20_2, DS18B20 *ds18b20_3)
{
    sensors[0].hx711 = hx711_1;
    sensors[0].ds18b20 = ds18b20_1;

    sensors[1].hx711 = hx711_2;
    sensors[1].ds18b20 = ds18b20_2;

    sensors[2].hx711 = hx711_3;
    sensors[2].ds18b20 = ds18b20_3;
}

void SensorsService::readSensors(uint8_t i) {
    logInfo("Sensor %d. Start read sensor\r\n", i);
    sensors[1].hx711->disable();
    HAL_Delay(1);
    bool isSensorPresent = sensors[i].ds18b20->start();
    if (isSensorPresent)
    {
        if (sensors[i].ds18b20->readRom())
        {
            sensors[i].isPresent = true;
            if (sensors[i].ds18b20->readTemperature())
            {
                logInfo("Sensor %d. Temperature: %d.%02d\r\n", i, (uint16_t)(sensors[i].ds18b20->getTemperature()), (uint16_t)(((uint16_t)(sensors[i].ds18b20->getTemperature() * 100)) % 100));
                int16_t coeficientInt = sensors[i].ds18b20->getScratchpad()[2] << 8 | sensors[i].ds18b20->getScratchpad()[3];
                logInfo("Sensor %d. Found coeficient int: %d\r\n", i, coeficientInt);
                if (coeficientInt != 0)
                {
                    float coeficient = coeficientInt / 100.0;
                    sensors[i].hx711->setCoeficient(coeficient);
                }
                sensors[i].hx711->powerUp();
                sensors[i].hx711->readRawValue(1);
                logInfo("Sensor %d. Raw value: %d\r\n", i, sensors[i].hx711->getRawValue());
                logInfo("Sensor %d. Weight: %d.%02d\r\n", i, (uint32_t)(sensors[i].hx711->getWeight()), (uint8_t)(((uint32_t)(sensors[i].hx711->getWeight() * 100)) % 100));
                sensors[i].hx711->powerDown();
            }
            else
            {
                logError("Sensor %d. Could not read temperature\r\n", i);
            }
        }
        else
        {
            logError("Sensor %d. Could not read Rom\r\n", i);
        }
    }
    else
    {
        logWarn("Sensor %d. Temperature sensor is not present\r\n", i);
    }
}

void SensorsService::readSensors()
{
    for (uint8_t i = 0; i < 3; i++)
    {
        readSensors(i);
    }
}

void SensorsService::resetBackUpRegisters()
{
    logInfo("Reset Backup registers\r\n");
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR6, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR7, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR8, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR9, 0x0);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR10, 0x0);
    HAL_PWR_DisableBkUpAccess();
}

void SensorsService::calibrateScales(bool buttonIsPressed)
{

    logInfo("Start callibration\r\n");
    for (uint8_t i = 0; i < 3; i++)
    {
        sensors[1].hx711->disable();
        if (HAL_RTCEx_BKUPRead(&hrtc, 3 * i + 1) == 0x0)
        {
            logInfo("Sensor %d. Start calibrate\r\n", i);
            bool isSensorPresent = sensors[i].ds18b20->start();
            if (isSensorPresent)
            {
                int32_t rawScaleValue = sensors[i].hx711->getAverageValue(1);
                logInfo("Sensor %d. Calibration in progress. Scale raw value: %d\r\n", i, rawScaleValue);
                HAL_PWR_EnableBkUpAccess();
                HAL_RTCEx_BKUPWrite(&hrtc, 3 * i + 1, 0x1);
                HAL_RTCEx_BKUPWrite(&hrtc, 3 * i + 3, 0xFFFF & rawScaleValue);
                HAL_RTCEx_BKUPWrite(&hrtc, 3 * i + 2, rawScaleValue >> 16);
                HAL_PWR_DisableBkUpAccess();
            }
            else
            {
                logWarn("Sensor %d. Temperature sensor is not present\r\n", i);
            }
        }
        else if (buttonIsPressed)
        {
            logInfo("Sensor %d. Back Up registers had data and button is pressed, let's calibrate it\r\n", i);
            bool isSensorPresent = sensors[i].ds18b20->start();
            if (isSensorPresent)
            {
                int32_t rawScaleValue = sensors[i].hx711->getAverageValue(5);
                logInfo("Sensor %d. Calibration in progress. Current raw value: %d\r\n", i, rawScaleValue);

                uint32_t valueM = HAL_RTCEx_BKUPRead(&hrtc, 3 * i + 2);
                uint32_t valueL = HAL_RTCEx_BKUPRead(&hrtc, 3 * i + 3);
                int32_t previousValue = valueM << 16 | (0xFFFF & valueL);
                logInfo("Sensor %d. Calibration in progress. Previous raw value: %d\r\n", i, previousValue);
                float coeficient = ((previousValue - rawScaleValue) * 100) / 1000; // 1Kg
                int16_t coeficientInt = (int16_t)coeficient;
                logInfo("Sensor %d. Calibration in progress. Calculated coeficient int: %d\r\n", i, coeficientInt);
                uint8_t tl = 0xFF & coeficientInt;
                uint8_t th = coeficientInt >> 8;
                if (sensors[i].ds18b20->saveBytes(th, tl))
                {
                    HAL_PWR_EnableBkUpAccess();
                    HAL_RTCEx_BKUPWrite(&hrtc, 3 * i + 1, 0x0);
                    HAL_RTCEx_BKUPWrite(&hrtc, 3 * i + 2, 0x0);
                    HAL_RTCEx_BKUPWrite(&hrtc, 3 * i + 3, 0x0);
                    HAL_PWR_DisableBkUpAccess();
                    logInfo("Sensor %d. Calibration in progress. Coificient is saved successful\r\n", i);
                }
                else
                {
                    logError("Sensor %d. Calibration in progress. Coificient is not saved\r\n", i);
                }
            }
            else
            {
                logWarn("Sensor %d. Temperature sensor is not present\r\n", i);
            }
        }
        else
        {
            logError("Sensor %d. Could not finish calibration becasue button is not pressed\r\n", i);
        }
    }
}

Sensor *SensorsService::getSensors()
{
    return sensors;
}