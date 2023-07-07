#include "alt_main.h"

#include "gpio.h"
#include "adc.h"
#include "spi.h"
#include "usart.h"
#include "rtc.h"
#include "tim.h"
#include "NRF24L01p.h"
#include "SIM800C.h"
#include "HX711.h"
#include "DS18B20.h"
#include "modem_service.h"
#include "led_service.h"
#include "sensors_service.h"
#include "radio_service.h"
#include <stdio.h>

#define NRF_CHANNEL 100
#define NRF_MASTER_ADDRESS 0x00001
#define NRF_SLAVE_ADDRESS 0x00002
#define NRF_SEND_SENSOR_1_DATA_CMD 0x01
#define NRF_SEND_SENSOR_2_DATA_CMD 0x02
#define NRF_SEND_SENSOR_3_DATA_CMD 0x03
#define NRF_SEND_GO_TO_SLEEP_CMD 0x04

void update();

NRF24L01p nRF24L01p(&hspi1, NRF_CE_GPIO_Port, NRF_CE_Pin, NRF_CSN_GPIO_Port, NRF_CSN_Pin);
SIM800C sim800c(&huart1, SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, SIM800C_DTR_GPIO_Port, SIM800C_DTR_Pin, &update);
HX711 hx711_1(&htim1, HX711_DT_1_GPIO_Port, HX711_DT_1_Pin, HX711_SCK_GPIO_Port, HX711_SCK_Pin);
HX711 hx711_2(&htim1, HX711_DT_2_GPIO_Port, HX711_DT_2_Pin, HX711_SCK_GPIO_Port, HX711_SCK_Pin);
HX711 hx711_3(&htim1, HX711_DT_3_GPIO_Port, HX711_DT_3_Pin, HX711_SCK_GPIO_Port, HX711_SCK_Pin);

DS18B20 ds18B20_1(&htim1, HX711_DT_1_GPIO_Port, HX711_DT_1_Pin);
DS18B20 ds18B20_2(&htim1, HX711_DT_2_GPIO_Port, HX711_DT_2_Pin);
DS18B20 ds18B20_3(&htim1, HX711_DT_3_GPIO_Port, HX711_DT_3_Pin);

SensorsService sensorsService(&hx711_1, &hx711_2, &hx711_3, &ds18B20_1, &ds18B20_2, &ds18B20_3);
ModemService modemService(&sim800c, &sensorsService, &update);
RadioService radioService(&nRF24L01p, &sensorsService, &update);

LedService ledService;

const uint8_t deviceAddress[5] = {0x00, 0x00, 0x00, 0x00, 0x01};

uint8_t data[32] = {0};
uint8_t sensorData[3][32] = {0};

bool deviceIsMaster = false;
bool deviceWasWakedUpFromA0 = false;
bool deviceWasWakedUpFromStandby = false;
bool deviceWasWakedUpFromPower = false;
bool buttonIsPressed = false;
uint8_t uart2RxBuffer[1];
uint32_t startDelayTick;

void nonBlockingDelay(uint32_t delayInTicks);
void printDeviceInfo();
void setLocalDateTime();
void setAlarmDateTime();
void handleReceiveDataEvent();
void goToStandByMode();
bool wakedUpFromA0();
bool wakedUpFromStandby();
bool wakedUpFromPower();
void handleModemResultStatus(ModemServiceResultStatus modemResultStatus, const char *message);
void populateSensorsData(uint8_t i);
void sendData(uint8_t i);
uint8_t getSlaveBatteryLevel();

int alt_main()
{
    buttonIsPressed = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_SET;
    HAL_TIM_Base_Start(&htim1);
    nonBlockingDelay(100);
    HAL_UART_Receive_IT(&huart2, uart2RxBuffer, 1);
    sim800c.init();
    deviceIsMaster = modemService.isSIM800CPresent();
    deviceWasWakedUpFromA0 = wakedUpFromA0();
    deviceWasWakedUpFromStandby = wakedUpFromStandby();
    deviceWasWakedUpFromPower = wakedUpFromPower();
    printDeviceInfo();
    ModemServiceResultStatus modemResultStatus;

    //sensorsService.readSensors();

    if (deviceWasWakedUpFromA0)
    {
        // TODO handle NRF IRQ, or alarm event
        printf("Waked Up from A0!\r\n");
        if (deviceIsMaster)
        {
        }
        else
        {
            if (nRF24L01p.isPowerUp() && nRF24L01p.isInRxMode()) {
                printf("It seems we were waked up by NRF IRQ\r\n");
                waitForNextCommand:
                while (true) {
                    if (nRF24L01p.isDataAvailable()) {
                        nRF24L01p.receive(data);
                        if (data[0] == NRF_SEND_SENSOR_1_DATA_CMD) {
                            populateSensorsData(0);
                            sendData(0);
                            goto waitForNextCommand;
                        } else if (data[0] == NRF_SEND_SENSOR_2_DATA_CMD) {
                            populateSensorsData(1);
                            sendData(1);
                            goto waitForNextCommand;
                        } else if (data[0] == NRF_SEND_SENSOR_3_DATA_CMD) {
                            populateSensorsData(2);
                            sendData(2);
                            goto waitForNextCommand;
                        } else if (data[0] == NRF_SEND_GO_TO_SLEEP_CMD) {
                            nRF24L01p.powerDown();
                            setLocalDateTime();
                            setAlarmDateTime();
                            goToStandByMode();
                        }
                    }
                }
            } else {
                printf("Waked Up from A0 and NRF doesn't have any data.\r\n");
            }
        }
    }
    else if (deviceWasWakedUpFromStandby) {
        // TODO handle NRF IRQ, or alarm event
        printf("Waked Up from stundby/Alarm!\r\n");
        if (deviceIsMaster)
        {
            printf("We are master let's start GSM, send master data and then send each slave data\r\n");

            ledService.blinkGreenLed(0, 300);
            
            modemResultStatus = modemService.startModemIfNeed();
            modemResultStatus = modemService.checkModemHealth();
            modemService.disablePowerOnPin();
            modemResultStatus = modemService.configureModem();
            modemResultStatus = modemService.findSMSWithSettingsAndConfigureModem();

            sensorsService.readSensors();
            populateSensorsData(0);
            populateSensorsData(1);
            populateSensorsData(2);

            modemResultStatus = modemService.sendData(sensorData);

            nRF24L01p.init();
            nRF24L01p.setTxMode();
            nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS, NRF_CHANNEL);
            data[0] = NRF_SEND_SENSOR_1_DATA_CMD;
            nRF24L01p.write(data);
            nRF24L01p.openReadingPipe(NRF_MASTER_ADDRESS, NRF_CHANNEL);
            while (true)
            {
                if (nRF24L01p.isDataAvailable()) {
                    nRF24L01p.receive(sensorData[0]);
                    break;
                }
            }

            nRF24L01p.setTxMode();
            nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS, NRF_CHANNEL);
            data[0] = NRF_SEND_SENSOR_2_DATA_CMD;
            nRF24L01p.write(data);
            nRF24L01p.openReadingPipe(NRF_MASTER_ADDRESS, NRF_CHANNEL);
            while (true)
            {
                if (nRF24L01p.isDataAvailable()) {
                    nRF24L01p.receive(sensorData[1]);
                    break;
                }
            }

            nRF24L01p.setTxMode();
            nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS, NRF_CHANNEL);
            data[0] = NRF_SEND_SENSOR_3_DATA_CMD;
            nRF24L01p.write(data);
            nRF24L01p.openReadingPipe(NRF_MASTER_ADDRESS, NRF_CHANNEL);
            while (true)
            {
                if (nRF24L01p.isDataAvailable()) {
                    nRF24L01p.receive(sensorData[2]);
                    break;
                }
            }

            nRF24L01p.setTxMode();
            nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS, NRF_CHANNEL);
            data[0] = NRF_SEND_GO_TO_SLEEP_CMD;


            RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();
            // date
            data[1] = localDate.Month;
            data[2] = localDate.Date;
            data[3] = localDate.Year;

            RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
            // time
            data[4] = localTime.Hours;
            data[5] = localTime.Minutes;
            data[6] = localTime.Seconds;

            // alarm
            data[7] = localTime.Hours;
            data[8] = localTime.Minutes;
            data[9] = localTime.Seconds;

            uint16_t minutes = modemService.getRefreshIntervalInMinutes();
            data[7] += minutes / 60;
            data[8] += minutes % 60;

            if (data[8] >= 60) {
                data[8] -= 60;
                data[7] += 1;
            }
            if (data[7] >= 24) {
                data[7] -= 24;
            }

            nRF24L01p.write(data);
            nRF24L01p.powerDown();

            modemResultStatus = modemService.sendData(sensorData);
            modemService.powerDown();
            setAlarmDateTime();
            goToStandByMode();
        }
        else
        {
            printf("We are slave, enable radio and go to sleep to wait commands!\r\n");
            nRF24L01p.init();
            nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, NRF_CHANNEL);
            goToStandByMode();
        }
    }
    else if (!deviceWasWakedUpFromPower)
    {
        printf("Waked Up from reset!\r\n");
        sensorsService.calibrateScales(buttonIsPressed);
    }
    else
    {
        // TODO Check modem and power on if need
        printf("Waked Up from power on!\r\n");
        if (deviceIsMaster)
        {
            ledService.blinkGreenLed(0, 300);

            modemResultStatus = modemService.startModemIfNeed();
            handleModemResultStatus(modemResultStatus, "Wasn't able to start SIM800C module");

            if (modemResultStatus == MODEM_SUCCESS)
            {
                modemResultStatus = modemService.checkModemHealth();
                handleModemResultStatus(modemResultStatus, "Wasn't able to check modem health");
            }

            modemService.disablePowerOnPin();

            if (modemResultStatus == MODEM_SUCCESS)
            {
                modemResultStatus = modemService.configureModem();
                handleModemResultStatus(modemResultStatus, "Wasn't able to configure modem");
            }

            if (modemResultStatus == MODEM_SUCCESS)
            {
                if (buttonIsPressed)
                {
                    printf("Button was pressed. Clear all SMS and wait for new settings\r\n");
                    modemResultStatus = modemService.deleteAllSMS();
                    handleModemResultStatus(modemResultStatus, "Wasn't able to delete all SMS");
                }
            }

            if (modemResultStatus == MODEM_SUCCESS)
            {
                modemResultStatus = modemService.findSMSWithSettingsAndConfigureModem();
            }

            if (modemResultStatus == MODEM_ERROR_SETTINGS_SMS_WASN_T_FOUND)
            {
                printf("Wasn't able to find Settings SMS. Wait for settings SMS\r\n");
                ledService.blinkGreenLed(0, 1000);
                modemResultStatus = modemService.waitForSettingsSMS();
                ledService.stopBlinkGreenLed();
                handleModemResultStatus(modemResultStatus, "Wasn't able to receive settings SMS");
            }

            if (modemResultStatus == MODEM_SUCCESS)
            {
                modemResultStatus = modemService.configureDateAndTime();
                handleModemResultStatus(modemResultStatus, "Wasn't able to configure date and time");
            }

            RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
            // alarm
            data[7] = localTime.Hours;
            data[8] = localTime.Minutes;
            data[9] = localTime.Seconds;

            data[9] += 4;
            if (data[9] >= 60) {
                data[9] -= 60;
                data[8] += 1;
            }
            if (data[8] >= 60) {
                data[8] -= 60;
                data[7] += 1;
            }
            if (data[7] >= 24) {
                data[7] -= 24;
            }
            setAlarmDateTime();
            goToStandByMode();
        }
        else
        {
            nRF24L01p.init();
            nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, NRF_CHANNEL);
            goToStandByMode();
        }
    }

    while (1)
    {
        update();
    }
}

uint8_t getSlaveBatteryLevel() {

    HAL_GPIO_WritePin(BATERY_LEVEL_CHECK_GPIO_Port, BATERY_LEVEL_CHECK_Pin, GPIO_PIN_RESET);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 1000); // poll for conversion 
    uint32_t value = HAL_ADC_GetValue(&hadc1); // get the adc value 
    HAL_ADC_Stop(&hadc1); // stop adc
    /*
    BAT      					4.2		3.2
    50
    ----------------			2.8		2.13
    100
    GND
    4096 ----- 3.3V
    x	 ----- 2.8V	
    y	 ----- 2.13V
    0	 ----- 0V
    3475 ---- 100%
    2643 ---- 0%
    832
    */
    return (uint8_t) ((value - 2643) / 832);
}

void populateSensorsData(uint8_t i) {
    printf("Populate sensor %d data\r\n", i);
    sensorsService.readSensors(i);
    for (uint8_t j = 0; j < 8; j++)
    {
        sensorData[i][j] = sensorsService.getSensors()[i].ds18b20->getRom()[j];                                       // ROM
    }       
    sensorData[i][8] = (uint8_t)(sensorsService.getSensors()[i].ds18b20->getTemperature());                           // TEMP_OUT
    sensorData[i][9] = (uint8_t)(((uint8_t)(sensorsService.getSensors()[i].ds18b20->getTemperature() * 100)) % 100);  // TEMP_OUT
    sensorData[i][10] = (uint32_t)(sensorsService.getSensors()[i].hx711->getWeight());                                // WEIGHT
    sensorData[i][11] = (uint8_t)(((uint32_t)(sensorsService.getSensors()[i].hx711->getWeight() * 100)) % 100);       // WEIGHT
    sensorData[i][12] = 29;                                                                                           // temp2
    sensorData[i][13] = 10;                                                                                           // temp2
    sensorData[i][14] = 50;                                                                                           // HUMEDITY
    sensorData[i][15] = 25;                                                                                           // HUMEDITY
    if (deviceIsMaster) {
        sensorData[i][16] = modemService.getBatteryLevel();                                                           // BATTERY LEVEL 0 - 100
    } else {
        sensorData[i][16] = getSlaveBatteryLevel();
    }    
}

void sendData(uint8_t i) {
    nRF24L01p.setTxMode();
    nRF24L01p.openWritingPipe(NRF_MASTER_ADDRESS, NRF_CHANNEL);
    nRF24L01p.write(sensorData[i]);
    nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, NRF_CHANNEL);
}

void nonBlockingDelay(uint32_t delayInTicks)
{
    startDelayTick = HAL_GetTick();
    while (true)
    {
        if (startDelayTick + delayInTicks < HAL_GetTick())
        {
            return;
        }
        update();
    }
}

void update()
{
    ledService.update();
}

void handleModemResultStatus(ModemServiceResultStatus modemResultStatus, const char *message)
{
    if (modemResultStatus != MODEM_SUCCESS)
    {
        printf("Error: %s\r\n", message);
        switch (modemResultStatus)
        {
        case MODEM_ERROR_IT_DIDN_T_REPONSD_AFTER_POWER_ON:
            ledService.blinkGreenLed(2);
            break;
        case MODEM_ERROR_SETTINGS_SMS_WASN_T_FOUND:
            ledService.blinkGreenLed(3);
            break;
        case MODEM_ERROR_RECEIVED_SMS_DOESN_T_CONTAINS_SETTINGS:
            ledService.blinkGreenLed(4);
            break;
        case MODEM_ERROR_SMS_RECEIVED_TIMEOUT:
            ledService.blinkGreenLed(5);
            break;
        case MODEM_ERROR_COULD_NOT_START_GPRS:
            ledService.blinkGreenLed(6);
            break;
        default:
            ledService.blinkGreenLed(1);
            break;
        }
    }
}

/*
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

        //nRF24L01p.printAllRegisters();

        //date
        data[0] = 0x02;
        data[1] = 0x02;
        data[2] = 0x02;

        // time
        data[3] = 0x02;
        data[4] = 0x02;
        data[5] = 0x01;

        // alarm
        data[6] = 0x02;
        data[7] = 0x02;
        data[8] = 0x0A;


        if (!deviceIsMaster) {
            if (nRF24L01p.isPowerUp() && nRF24L01p.isInRxMode() && nRF24L01p.isDataAvailable()) {
                // waked up by NRF IRQ
                printf("It seems we were waked up by NRF IRQ\r\n");
                handleReceiveDataEvent();

            } else if ((PWR->CSR) & (PWR_CSR_SBF)) {

                printf("It seems we were waked up from standby\r\n");
                // waked up by RTC alarm
                // TODO move NRF to RX and go standby

                RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();

                // time
                data[6] = localTime.Hours;
                data[7] = localTime.Minutes;
                data[8] = localTime.Seconds;

                data[8] += 10;
                if (data[8] >= 60) {
                    data[8] -= 60;
                    data[7] += 1;
                }
                if (data[7] >= 60) {
                    data[7] -= 60;
                    data[6] += 1;
                }

                if (data[6] >= 24) {
                    data[6] -= 24;
                }

                // TODO check years;
                setAlarmDateTime();
                HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
                HAL_Delay(2000);
                goToStandByMode();
            } else {
                printf("It seems we were waked up by power on\r\n");
                // TODO check current time
                // Configure NRF to RX mode
                // if date/time is not configured wait packet. Configure NRF to RX mode


                //while(!nRF24L01p.isDataAvailable()) {
                    // handle LED blink;
                //}
                handleReceiveDataEvent();


                setLocalDateTime();
                setAlarmDateTime();
                HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
                HAL_Delay(2000);
                goToStandByMode();

            }
        }


        printf("Error!!!\r\n");
        */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        sim800c.txCpltCallback();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        sim800c.rxCpltCallback();
    }
    if (huart->Instance == USART2)
    {
        // HAL_UART_Transmit(&huart1, uart2RxBuffer, 1, 100);
        HAL_UART_Receive_IT(&huart2, uart2RxBuffer, 1);
    }
}

void printDeviceInfo()
{
    printf("\r\n");
    printf("Bees Scale Device powered by Eugen Scobich\r\n");
    printf("Device Address: 0x%0X%0X%0X%0X%0X\r\n", deviceAddress[0], deviceAddress[1], deviceAddress[2], deviceAddress[3], deviceAddress[4]);
    printf("Device Mode: %s\r\n", deviceIsMaster ? "Master" : "Slave");
    printf("Waked up from: %s\r\n", deviceWasWakedUpFromStandby ? "Standby" : deviceWasWakedUpFromPower ? "Power On"
                                                                                                        : "Reset");
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();

    printf("Device Date: %02d.%02d.%02d\r\n", localDate.Date, localDate.Month, localDate.Year);
    printf("Device Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
}

void setLocalDateTime()
{
    // 1:3 bytes contains current date
    // 4:6 bytes contains current time
    HAL_RTC_SetLocalDate(data[1], data[2], data[3]);
    RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();
    printf("Set Date: %02d.%02d.%02d\r\n", localDate.Date, localDate.Month, localDate.Year);

    HAL_RTC_SetLocalTime(data[4], data[5], data[6]);
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    printf("Set Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
}

void setAlarmDateTime()
{
    // 7:9 bytes contains alarm time
    HAL_RTC_SetLocalAlarm(data[7], data[8], data[9]);
    RTC_TimeTypeDef localAlarm = HAL_RTC_GetLocalAlarm();
    printf("Set Alarm Time: %02d:%02d:%02d\r\n", localAlarm.Hours, localAlarm.Minutes, localAlarm.Seconds);
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    printf("Alarm!!!\r\n");
}

void handleReceiveDataEvent()
{
    nRF24L01p.receive(data);

    if (data[0] == 0x01) {
        printf("Received command to send sensor 1 details!!!\r\n");

    }



    setLocalDateTime();
    setAlarmDateTime();

    // data 0 MCU temp
    // data 1:2 Bat voltage
    // data 3:7 Reserved

    // data 8:9 scale sensor 1
    // data 10:11 temp sensor 1
    // data 12:13 humidity sensor 1
    // data 14:15 reserved

    // data 16:17 scale sensor 1
    // data 18:19 temp sensor 2
    // data 20:21 humidity sensor 2
    // data 22:23 reserved

    // data 24:25 scale sensor 3
    // data 26:27 temp sensor 3
    // data 28:29 humidity sensor 3
    // data 30:31 reserved

    // TODO check sensors
    // TODO return data

    // TODO turn NRF off
    // TODO go standby
}

void goToStandByMode()
{
    printf("Go to StandBy Mode\r\n");
    ledService.blinkGreenRedOrangeLedOneTime();
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN);
    PWR->CR |= PWR_CR_CWUF;
    PWR->CSR |= PWR_CSR_EWUP;
    PWR->CR |= PWR_CR_CSBF;
    HAL_PWR_EnterSTANDBYMode();
}

bool wakedUpFromA0()
{
    return (PWR->CSR) & (PWR_CSR_WUF);
}

bool wakedUpFromStandby()
{
    return (PWR->CSR) & (PWR_CSR_SBF);
}

bool wakedUpFromPower()
{
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
    {
        __HAL_RCC_CLEAR_RESET_FLAGS();
        return true;
    }
    return false;
}