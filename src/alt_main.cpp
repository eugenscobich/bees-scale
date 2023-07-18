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
#include "time_service.h"
#include <stdio.h>

#define CLASS_NAME "alt_main."
#include "log.h"

#define NRF_MASTER_ADDRESS (uint32_t)0x32AB12E7
#define NRF_SLAVE_ADDRESS_1 (uint32_t)0x32AB12E8
#define NRF_SLAVE_ADDRESS_2 (uint32_t)0x32AB12E9
#define NRF_SLAVE_ADDRESS_3 (uint32_t)0x32AB12EA
#define NRF_SLAVE_ADDRESS_4 (uint32_t)0x32AB12EB
#define NRF_SLAVE_ADDRESS_5 (uint32_t)0x32AB12EC
#define NRF_SLAVE_ADDRESS_6 (uint32_t)0x32AB12ED
#define NRF_SLAVE_ADDRESS_7 (uint32_t)0x32AB12EE
#define NRF_SLAVE_ADDRESS_8 (uint32_t)0x32AB12EF
#define NRF_SLAVE_ADDRESS_9 (uint32_t)0x32AB12F1
#define NRF_SLAVE_ADDRESS_10 (uint32_t)0632AB12F2

#define SERVICE_ID 1
#define SETTINGS_SMS_DIDN_T_RECIEVED_ERROR_CODE 1

void update();

NRF24L01p nRF24L01p(&hspi1, NRF_CE_GPIO_Port, NRF_CE_Pin, NRF_CSN_GPIO_Port, NRF_CSN_Pin, &update);
SIM800C sim800c(&huart1, SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, SIM800C_DTR_GPIO_Port, SIM800C_DTR_Pin, &update);
HX711 hx711_1(&htim1, HX711_DT_1_GPIO_Port, HX711_DT_1_Pin, HX711_SCK_GPIO_Port, HX711_SCK_Pin);
HX711 hx711_2(&htim1, HX711_DT_2_GPIO_Port, HX711_DT_2_Pin, HX711_SCK_GPIO_Port, HX711_SCK_Pin);
HX711 hx711_3(&htim1, HX711_DT_3_GPIO_Port, HX711_DT_3_Pin, HX711_SCK_GPIO_Port, HX711_SCK_Pin);

DS18B20 ds18B20_1(&htim1, HX711_DT_1_GPIO_Port, HX711_DT_1_Pin);
DS18B20 ds18B20_2(&htim1, HX711_DT_2_GPIO_Port, HX711_DT_2_Pin);
DS18B20 ds18B20_3(&htim1, HX711_DT_3_GPIO_Port, HX711_DT_3_Pin);

void error(uint8_t serviceId, uint8_t errorCode);

SensorsService sensorsService(&hx711_1, &hx711_2, &hx711_3, &ds18B20_1, &ds18B20_2, &ds18B20_3);
ModemService modemService(&sim800c, &sensorsService, &update, &error);
RadioService radioService(&nRF24L01p, &sensorsService, &update);

LedService ledService;
TimeService timeService;

uint8_t data[32] = {0};
uint8_t masterData[3][32] = {0};
uint8_t sensorData[3][32] = {0};

bool deviceIsMaster = false;
bool deviceWasWakedUpFromStandby = false;
bool deviceWasWakedUpFromPower = false;
bool isButtonPressed = false;
uint8_t uart2RxBuffer[1];
uint32_t startDelayTick;

void nonBlockingDelay(uint32_t delayInTicks);
void printDeviceInfo();
void goToStandByMode();
bool wakedUpFromA0();
bool wakedUpFromStandby();
bool wakedUpFromPower();
bool wakedUpFromPinReset();
bool wakedUpFromSoftware();
void readSensorAndPopulateSensorData(uint8_t i);
void sendData(uint8_t i);
uint8_t getSlaveBatteryLevel();
void printData(uint8_t *dataToPrint);

void initNrfOnPowerOn();
float getCpuTemperature();

int alt_main()
{
    isButtonPressed = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_SET;
    HAL_TIM_Base_Start(&htim1);
    HAL_UART_Receive_IT(&huart2, uart2RxBuffer, 1);

    sim800c.init();
    nRF24L01p.isPowerUp(); // hack to init SPI after sleep
    deviceIsMaster = modemService.isModemPresent();
    deviceWasWakedUpFromStandby = wakedUpFromStandby();
    deviceWasWakedUpFromPower = wakedUpFromPower();
    printDeviceInfo();

    // ===================================== Sandbox

    // =====================================  Sandbox

    if (deviceWasWakedUpFromStandby)
    {
        if (radioService.isRadioInRxMode())
        {
            logInfo("Waked Up by Radio\r\n");
            if (deviceIsMaster)
            {
                logError("Waked Up from Radio, but we are master, this is not expected\r\n");
                goToStandByMode();
            }
            else
            {
                if (radioService.isDataReceived())
                {
                    radioService.readMasterData(masterData);
                    timeService.setDateAndTime(masterData[0]);
                    timeService.setAlarmTime(masterData[0]);
                    radioService.powerDown();
                    goToStandByMode();
                }
                else
                {
                    logWarn("Radio is UP but there is no data\r\n");
                    goToStandByMode();
                }
            }
        }
        else
        {
            logInfo("Waked up by alarm\r\n");
            if (deviceIsMaster)
            {
                logInfo("We are master let's start GSM, send master data and then send each slave data\r\n");

                ledService.blinkGreenLed(0, 400);

                modemService.startModemIfNeed();
                modemService.checkModemHealth();
                modemService.disablePowerOnPin();

                modemService.configureModem();
                modemService.findSMSWithSettingsAndConfigureModem();

                readSensorAndPopulateSensorData(0);
                readSensorAndPopulateSensorData(1);
                readSensorAndPopulateSensorData(2);

                modemService.sendData(sensorData);

                logInfo("Master data was sent successful\r\n");

                logInfo("Ask Slave 1, Sensor one details.\r\n");
                logInfo("Init NRF\r\n");
                nRF24L01p.init();
                nRF24L01p.enablePayloadWithAknoladge();
                nRF24L01p.enableDynamicPayload(0);
                nRF24L01p.enableDynamicPayload(1);
                nRF24L01p.stopListening();
                nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS_1);

                timeService.populateDataWithDateAndTime(data);
                timeService.populateDataWithAlarmTimeFor(data, 30, SECONDS);
                logInfo("Send ");
                printData(data);

                for (uint8_t i = 0; i < 3; i++)
                {
                    bool successSendCmd = nRF24L01p.write(data);
                    if (successSendCmd)
                    {
                        if (nRF24L01p.isDataAvailable())
                        {
                            nRF24L01p.receive(sensorData[i]);
                            logInfo("Received ");
                            printData(sensorData[i]);
                            nonBlockingDelay(40); // To allow slave to receive data and print it
                        }
                        else
                        {
                            logInfo("No Data\r\n");
                        }
                    }
                    else
                    {
                        logInfo("Send CMD failed: 0x%02X\r\n", data[0]);
                    }
                }

                nRF24L01p.powerDown();
                modemService.sendData(sensorData);
                logInfo("Slave data was sent successful\r\n");
                modemService.powerDown();
                timeService.setAlarmFor(30, SECONDS);
                goToStandByMode();
            }
            else
            {
                initNrfOnPowerOn();
            }
        }
    }
    else if (!deviceWasWakedUpFromPower)
    {
        logInfo("Waked Up from reset!\r\n");
        sensorsService.calibrateScales(isButtonPressed);
    }
    else
    {
        // TODO Check modem and power on if need
        logInfo("Waked Up from power on!\r\n");
        if (deviceIsMaster)
        {
            ledService.blinkGreenLed(0, 300);
            modemService.startModemIfNeed();
            modemService.checkModemHealth();
            modemService.disablePowerOnPin();
            modemService.configureModem();
            if (isButtonPressed)
            {
                logInfo("Button was pressed. Clear all SMS and wait for new settings\r\n");
                modemService.deleteAllSMS();
            }

            modemService.findSMSWithSettingsAndConfigureModem();

            if (!modemService.isSettingsSMSFound())
            {
                logWarn("Wasn't able to find Settings SMS. Wait for settings SMS\r\n");
                ledService.blinkGreenLed(0, 1000);
                modemService.waitForSettingsSMS();
                ledService.stopBlinkGreenLed();
                if (!modemService.isSettingsSMSFound())
                {
                    logError("Settings SMS didn't recieved\r\n");
                    error(SERVICE_ID, SETTINGS_SMS_DIDN_T_RECIEVED_ERROR_CODE);
                }
            }

            modemService.configureDateAndTime();
            timeService.setAlarmFor(10, SECONDS);
            goToStandByMode();
        }
        else
        {
            initNrfOnPowerOn();
        }
    }

    while (1)
    {
        update();
    }
}

void initNrfOnPowerOn()
{
    logInfo("We are slave, read sensors, enable radio, set ack packages and go to sleep to wait commands!\r\n");
    readSensorAndPopulateSensorData(0);
    readSensorAndPopulateSensorData(1);
    readSensorAndPopulateSensorData(2);

    printf("Init NRF\r\n");
    nRF24L01p.init();
    nRF24L01p.enablePayloadWithAknoladge();
    nRF24L01p.enableDynamicPayload(0);
    nRF24L01p.enableDynamicPayload(1);
    nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS_1, RX_PIPE_1);
    nRF24L01p.printAllRegisters();

    nRF24L01p.flushTx();
    logInfo("TX is full: [%d], ", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[0], 32));
    printData(sensorData[0]);
    logInfo("TX is full: [%d], ", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[1], 32));
    printData(sensorData[1]);
    logInfo("TX is full: [%d], ", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[2], 32));
    printData(sensorData[2]);

    nRF24L01p.startListening();
    goToStandByMode();
}

float getCpuTemperature()
{
    // HAL_ADCEx_Calibration_Start(&hadc1);
    Set_ADC1_Channel(ADC_CHANNEL_TEMPSENSOR);
    HAL_GPIO_WritePin(BATERY_LEVEL_CHECK_GPIO_Port, BATERY_LEVEL_CHECK_Pin, GPIO_PIN_RESET);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY); // poll for conversion
    uint16_t value = HAL_ADC_GetValue(&hadc1);        // get the adc value
    HAL_ADC_Stop(&hadc1);                             // stop adc
    logInfo("Raw adc value: %d\r\n", value);

    float temp = ((((value * 3.3 / 4096) - 1.43) / 0.0043) + 25);
    logInfo("CPU Temp: %d\r\n", (int16_t)(temp * 100.0));
    return temp;
}

uint8_t getSlaveBatteryLevel()
{
    // HAL_ADCEx_Calibration_Start(&hadc1);
    Set_ADC1_Channel(ADC_CHANNEL_1);
    HAL_GPIO_WritePin(BATERY_LEVEL_CHECK_GPIO_Port, BATERY_LEVEL_CHECK_Pin, GPIO_PIN_RESET);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY); // poll for conversion
    uint16_t value = HAL_ADC_GetValue(&hadc1);        // get the adc value
    logInfo("Raw adc value: %d\r\n", value);

    uint32_t voltage = ((3.3 * value) / 4098) * 1000;
    logInfo("Voltage: %lu\r\n", voltage);

    HAL_ADC_Stop(&hadc1); // stop adc
    HAL_GPIO_WritePin(BATERY_LEVEL_CHECK_GPIO_Port, BATERY_LEVEL_CHECK_Pin, GPIO_PIN_SET);
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
    uint8_t batteryLevel = (uint8_t)(((value - 2643.0) / 832.0) * 100);
    logInfo("Slave Battery level is: %d\r\n", batteryLevel);

    getCpuTemperature();

    return batteryLevel;
}

void readSensorAndPopulateSensorData(uint8_t i)
{
    logInfo("Populate sensor %d data\r\n", i);
    sensorsService.readSensors(i);
    if (sensorsService.getSensors()[i].isPresent)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            sensorData[i][j] = sensorsService.getSensors()[i].ds18b20->getRom()[j]; // ROM
        }
        sensorData[i][8] = (uint8_t)(sensorsService.getSensors()[i].ds18b20->getTemperature());                           // TEMP_OUT
        sensorData[i][9] = (uint8_t)(((uint16_t)(sensorsService.getSensors()[i].ds18b20->getTemperature() * 100)) % 100); // TEMP_OUT

        uint32_t weight = (uint32_t)(sensorsService.getSensors()[i].hx711->getWeight());

        sensorData[i][10] = (uint8_t)(weight >> 24);                                                                // WEIGHT  0xA1B2C3D4 -> 0xA1
        sensorData[i][11] = (uint8_t)((weight >> 16) & 0xFF);                                                       // WEIGHT  0xA1B2C3D4 -> 0xA1B2 -> 0xB2
        sensorData[i][12] = (uint8_t)((weight >> 8) & 0xFF);                                                        // WEIGHT  0xA1B2C3D4 -> 0xA1B2C3 -> 0xC3
        sensorData[i][13] = (uint8_t)(weight & 0xFF);                                                               // WEIGHT  0xA1B2C3D4 -> 0xA1B2C3D4 -> 0xD4
        sensorData[i][14] = (uint8_t)(((uint32_t)(sensorsService.getSensors()[i].hx711->getWeight() * 100)) % 100); // WEIGHT

        sensorData[i][15] = 29; // temp2
        sensorData[i][16] = 10; // temp2
        sensorData[i][17] = 50; // HUMEDITY
        sensorData[i][18] = 25; // HUMEDITY
        if (deviceIsMaster)
        {
            sensorData[i][19] = modemService.getBatteryLevel(); // BATTERY LEVEL 0 - 100
        }
        else
        {
            sensorData[i][19] = getSlaveBatteryLevel();
        }
    }
    else
    {
        for (uint8_t j = 0; j < 32; j++)
        {
            sensorData[i][j] = 0;
        }
    }
}

void sendData(uint8_t i)
{
    nRF24L01p.stopListening();
    nRF24L01p.openWritingPipe(NRF_MASTER_ADDRESS);
    nRF24L01p.write(sensorData[i]);
    nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS_1, 1);
    nRF24L01p.startListening();
}

void printData(uint8_t *dataToPrint)
{
    printf("Data: 0x");
    for (uint8_t i = 0; i < 32; i++)
    {
        printf("%02X", dataToPrint[i]);
    }
    printf("\r\n");
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

void error(uint8_t serviceId, uint8_t errorCode)
{
    logError("Error %u in service: %u\r\n", errorCode, serviceId);
    ledService.blinkRedAndOrangeLed(serviceId, errorCode, 400, 3000);
    nonBlockingDelay(30000);
    timeService.setAlarmFor(1, MINUTES);
    goToStandByMode();
}

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
    logInfo("=====================================================================================\r\n");
    logInfo("Bees Scale Device powered by Eugen Scobich\r\n");
    logInfo("Device Address: ");
    if (deviceIsMaster)
    {
        printf("0x%0lX\r\n", NRF_MASTER_ADDRESS);
    }
    else
    {
        printf("0x%0lX\r\n", NRF_SLAVE_ADDRESS_1);
    }

    logInfo("Device Mode: %s\r\n", deviceIsMaster ? "Master" : "Slave");
    logInfo("Waked up from: %s\r\n", deviceWasWakedUpFromStandby ? "Standby" : deviceWasWakedUpFromPower ? "Power On"
                                                                                                         : "Reset");
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();

    logInfo("Device Date: %02d.%02d.%02d\r\n", localDate.Date, localDate.Month, localDate.Year);
    logInfo("Device Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
    logInfo("=====================================================================================\r\n");
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    logInfo("Alarm!!!\r\n");
}

void goToStandByMode()
{
    logInfo("Go to StandBy Mode\r\n");
    ledService.blinkGreenRedOrangeLedOneTime();
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);        // PWR->CR |= PWR_CR_CWUF;
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);        // PWR->CR |= PWR_CR_CSBF;
    __HAL_RCC_PWR_CLK_ENABLE();               // RCC->APB1ENR |= (RCC_APB1ENR_PWREN);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1); // PWR->CSR |= PWR_CSR_EWUP;
    HAL_PWR_EnterSTANDBYMode();
}

bool wakedUpFromA0()
{
    return __HAL_PWR_GET_FLAG(PWR_FLAG_WU);
    // return (PWR->CSR) & (PWR_CSR_WUF);
}

bool wakedUpFromStandby()
{
    return __HAL_PWR_GET_FLAG(PWR_FLAG_SB);
    // return (PWR->CSR) & (PWR_CSR_SBF);
}

bool wakedUpFromSoftware()
{
    return RCC->CSR & RCC_CSR_SFTRSTF;
}

bool wakedUpFromPinReset()
{
    return RCC->CSR & RCC_CSR_PINRSTF;
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