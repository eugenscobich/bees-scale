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

#define NRF_MASTER_ADDRESS 0x1632A
#define NRF_SLAVE_ADDRESS 0x1632B
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
TimeService timeService;

const uint8_t deviceAddress[5] = {0x00, 0x00, 0x00, 0x00, 0x01};

uint8_t data[32] = {0};
uint8_t sensorData[3][32] = {0};

bool deviceIsMaster = false;
bool deviceWasWakedUpFromStandby = false;
bool deviceWasWakedUpFromPower = false;
bool buttonIsPressed = false;
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
void handleModemResultStatus(ModemServiceResultStatus modemResultStatus, const char *message);
void readSensorAndPopulateSensorData(uint8_t i);
void sendData(uint8_t i);
uint8_t getSlaveBatteryLevel();
void printData(uint8_t *dataToPrint);

void initNrfOnPowerOn();
float getCpuTemperature() ;

int alt_main()
{
    buttonIsPressed = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_SET;
    HAL_TIM_Base_Start(&htim1);
    // nonBlockingDelay(100);
    HAL_UART_Receive_IT(&huart2, uart2RxBuffer, 1);
    sim800c.init();
    deviceIsMaster = modemService.isSIM800CPresent();
    deviceWasWakedUpFromStandby = wakedUpFromStandby();
    deviceWasWakedUpFromPower = wakedUpFromPower();
    printDeviceInfo();
    ModemServiceResultStatus modemResultStatus;
    /*
        // ================ Sandbox
        if (deviceIsMaster)
        {
            nRF24L01p.init();
            // nRF24L01p.enablePayloadWithAknoladge();
            // nRF24L01p.enableDynamicPayload(0);
            // nRF24L01p.enableDynamicPayload(1);

            for (uint8_t i = 0; i < 3; i++)
            {
                nRF24L01p.stopListening();
                nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS);
                uint8_t data1[32] = {i, 0x01, 0x01, 0x00, 0x00, 0x04, 0x0D, 0x00, 0x04, 0x2B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                printf("Send Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                       data1[0], data1[1], data1[2], data1[3], data1[4], data1[5], data1[6], data1[7],
                       data1[8], data1[9], data1[10], data1[11], data1[12], data1[13], data1[14], data1[15],
                       data1[16], data1[17], data1[18], data1[19]);

                bool successSendCmd = nRF24L01p.write(data1);
                if (successSendCmd)
                {
                    nRF24L01p.openReadingPipe(NRF_MASTER_ADDRESS, RX_PIPE_1);
                    nRF24L01p.startListening();
                    startDelayTick = HAL_GetTick();
                    while (true)
                    {
                        if (nRF24L01p.isDataReceived())
                        {
                            nRF24L01p.receive(data);
                            printf("Received Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                                   data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                                   data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15],
                                   data[16], data[17], data[18], data[19]);
                            break;
                        }
                        else if (startDelayTick + 3000 < HAL_GetTick())
                        {
                            printf("Timeout\r\n");
                            break;
                        }
                    }
                }
                else
                {
                    printf("Send Data failed: 0x%02X\r\n", data1[0]);
                }
            }

            // nRF24L01p.printAllRegisters();
            nRF24L01p.powerDown();
            goToStandByMode();
        }
        else
        {
            nRF24L01p.isPowerUp();
            if (nRF24L01p.isPowerUp() && nRF24L01p.isInRxMode())
            {
                if (nRF24L01p.isDataReceived())
                {
                    startDelayTick = HAL_GetTick();
                    while (true)
                    {
                        if (nRF24L01p.isDataReceived())
                        {
                            nRF24L01p.receive(data);
                            printf("Received Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                                    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                                    data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15],
                                    data[16], data[17], data[18], data[19]);

                            readSensorAndPopulateData(data[0]);

                            nRF24L01p.stopListening();
                            nRF24L01p.openWritingPipe(NRF_MASTER_ADDRESS);

                            bool successSendCmd = false;
                            if (data[0] == 2) {
                                uint8_t data1[32] = {0x28, 0xF0, 0x5E, 0xB5, 0x05, 0x00, 0x00, 0x7E, 0x1B, 0x06, 0x00, 0x00, 0xAC, 0xDE, 0x46, 0x1D, 0x0A, 0x32, 0x19, 0xEA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
                                successSendCmd = nRF24L01p.write(data1);
                            } else {
                                uint8_t data1[32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x0A, 0x32, 0x19, 0xEA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
                                successSendCmd = nRF24L01p.write(data1);
                            }

                            if (successSendCmd)
                            {
                                if (data[0] == 2) {
                                    printf("Session ended\r\n");
                                    break;
                                } else {
                                    printf("Continue listening\r\n");
                                    nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, RX_PIPE_1);
                                    nRF24L01p.startListening();
                                }
                            }
                            else
                            {
                                printf("Send Data failed: 0x%02X\r\n", data[0]);
                            }

                        }
                        else if (startDelayTick + 10000 < HAL_GetTick())
                        {
                            printf("Timeout\r\n");
                            break;
                        }

                    }

                    nRF24L01p.powerDown();
                    goToStandByMode();
                }
                else
                {
                    printf("NRF is UP and RX but no data\r\n");
                }
            }
            else
            {
                printf("Init NRF\r\n");
                nRF24L01p.init();
                // nRF24L01p.enablePayloadWithAknoladge();
                // nRF24L01p.enableDynamicPayload(0);
                // nRF24L01p.enableDynamicPayload(1);
                nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, RX_PIPE_1);

                                nRF24L01p.flushTx();
                                //0x0000000000000000000000000000001D0A3219EA
                                uint8_t data1[32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x0A, 0x32, 0x19, 0xEA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
                                printf("Write Acknowledge Payload: %d\r\n", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, data1, 32));
                                printf("Write Acknowledge Payload: %d\r\n", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, data1, 32));
                                //0x28F05EB50500007E1B060000ACDE461D0A3219EA
                                uint8_t data2[32] = {0x28, 0xF0, 0x5E, 0xB5, 0x05, 0x00, 0x00, 0x7E, 0x1B, 0x06, 0x00, 0x00, 0xAC, 0xDE, 0x46, 0x1D, 0x0A, 0x32, 0x19, 0xEA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
                                printf("Write Acknowledge Payload: %d\r\n", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, data2, 32));

                nRF24L01p.startListening();

                // nRF24L01p.printAllRegisters();
            }
        }
        goToStandByMode();
        // =====================================  Sandbox
    */
    if (deviceWasWakedUpFromStandby)
    {
        nRF24L01p.isPowerUp();
        if (nRF24L01p.isPowerUp() && nRF24L01p.isInRxMode())
        {
            // TODO handle NRF IRQ, or alarm event
            printf("Waked Up by NRF\r\n");
            if (deviceIsMaster)
            {
                printf("Waked Up from NRF, but we are master, this is not expected!\r\n");
            }
            else
            {
                printf("It seems we were waked up by NRF IRQ\r\n");
                if (nRF24L01p.isDataReceived())
                {
                    startDelayTick = HAL_GetTick();
                    while (true)
                    {
                        if (nRF24L01p.isDataReceived())
                        {
                            nRF24L01p.receive(data);
                            printf("Received ");
                            printData(data);
                        }
                        else if (startDelayTick + 200 < HAL_GetTick())
                        {
                            printf("Timeout Receive data\r\n");
                            break;
                        }
                    }
                    timeService.setDateAndTime(data);
                    timeService.setAlarmTime(data);
                    nRF24L01p.powerDown();
                    goToStandByMode();
                }
                else
                {
                    printf("NRF is UP but no data!\r\n");
                    goToStandByMode();
                }
            }
        }
        else
        {
            printf("Waked Up by Alarm!\r\n");
            if (deviceIsMaster)
            {
                printf("We are master let's start GSM, send master data and then send each slave data\r\n");

                ledService.blinkGreenLed(0, 300);

                modemResultStatus = modemService.startModemIfNeed();
                modemResultStatus = modemService.checkModemHealth();
                modemService.disablePowerOnPin();
                modemResultStatus = modemService.configureModem();
                modemResultStatus = modemService.findSMSWithSettingsAndConfigureModem();

                readSensorAndPopulateSensorData(0);
                readSensorAndPopulateSensorData(1);
                readSensorAndPopulateSensorData(2);

                modemResultStatus = modemService.sendData(sensorData);
                
                printf("Master data was sent successful\r\n");

                printf("Ask Slave 1, Sensor one details.\r\n");
                printf("Init NRF\r\n");
                nRF24L01p.init();
                nRF24L01p.enablePayloadWithAknoladge();
                nRF24L01p.enableDynamicPayload(0);
                nRF24L01p.enableDynamicPayload(1);
                nRF24L01p.stopListening();
                nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS);

                timeService.populateDataWithDateAndTime(data);
                timeService.populateDataWithAlarmTimeFor(data, 30, SECONDS);
                printf("Send ");
                printData(data);

                for (uint8_t i = 0; i < 3; i++)
                {
                    bool successSendCmd = nRF24L01p.write(data);
                    if (successSendCmd)
                    {
                        if (nRF24L01p.isDataAvailable())
                        {
                            nRF24L01p.receive(sensorData[i]);
                            printf("Received ");
                            printData(sensorData[i]);
                        }
                        else
                        {
                            printf("No Data\r\n");
                        }
                    }
                    else
                    {
                        printf("Send CMD failed: 0x%02X\r\n", data[0]);
                    }
                }

                nRF24L01p.powerDown();
                modemResultStatus = modemService.sendData(sensorData);
                printf("Slave data was sent successful\r\n");
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

void initNrfOnPowerOn() {
    printf("We are slave, read sensors, enable radio, set ack packages and go to sleep to wait commands!\r\n");
    readSensorAndPopulateSensorData(0);
    readSensorAndPopulateSensorData(1);
    readSensorAndPopulateSensorData(2);

    printf("Init NRF\r\n");
    nRF24L01p.init();
    nRF24L01p.enablePayloadWithAknoladge();
    nRF24L01p.enableDynamicPayload(0);
    nRF24L01p.enableDynamicPayload(1);
    nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, RX_PIPE_1);

    nRF24L01p.flushTx();
    printf("TX is full: [%d], ", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[0], 32));
    printData(sensorData[0]);
    printf("TX is full: [%d], ", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[1], 32));
    printData(sensorData[1]);
    printf("TX is full: [%d], ", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[2], 32));
    printData(sensorData[2]);

    nRF24L01p.startListening();
    goToStandByMode();
}

float getCpuTemperature() {
    //HAL_ADCEx_Calibration_Start(&hadc1);
    Set_ADC1_Channel(ADC_CHANNEL_TEMPSENSOR);
    HAL_GPIO_WritePin(BATERY_LEVEL_CHECK_GPIO_Port, BATERY_LEVEL_CHECK_Pin, GPIO_PIN_RESET);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);   // poll for conversion
    uint16_t value = HAL_ADC_GetValue(&hadc1); // get the adc value
    HAL_ADC_Stop(&hadc1);                      // stop adc
    printf("Raw adc value: %d\r\n", value);

    float temp = ((((value * 3.3 / 4096) - 1.43) / 0.0043) + 25);
    printf("CPU Temp: %d\r\n", (int16_t)(temp * 100.0));
    return temp;
}

uint8_t getSlaveBatteryLevel()
{
    //HAL_ADCEx_Calibration_Start(&hadc1);
    Set_ADC1_Channel(ADC_CHANNEL_1);
    HAL_GPIO_WritePin(BATERY_LEVEL_CHECK_GPIO_Port, BATERY_LEVEL_CHECK_Pin, GPIO_PIN_RESET);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);   // poll for conversion
    uint16_t value = HAL_ADC_GetValue(&hadc1); // get the adc value

    printf("Raw adc value: %d\r\n", value);
    printf("Voltage: %d\r\n", (uint32_t)(((3.3 * value) / 4098) * 1000));
    HAL_ADC_Stop(&hadc1);                      // stop adc
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
    printf("Slave Battery level is: %d\r\n", batteryLevel);

    getCpuTemperature();

    return batteryLevel;
}

void readSensorAndPopulateSensorData(uint8_t i)
{
    printf("Populate sensor %d data\r\n", i);
    sensorsService.readSensors(i);
    if (sensorsService.getSensors()[i].isPresent) {
        for (uint8_t j = 0; j < 8; j++)
        {
            sensorData[i][j] = sensorsService.getSensors()[i].ds18b20->getRom()[j]; // ROM
        }
        sensorData[i][8] = (uint8_t)(sensorsService.getSensors()[i].ds18b20->getTemperature());                           // TEMP_OUT
        sensorData[i][9] = (uint8_t)(((uint16_t)(sensorsService.getSensors()[i].ds18b20->getTemperature() * 100)) % 100); // TEMP_OUT

        uint32_t weight = (uint32_t)(sensorsService.getSensors()[i].hx711->getWeight());

        sensorData[i][10] = (uint8_t)(weight >> 24);                                                                      // WEIGHT  0xA1B2C3D4 -> 0xA1
        sensorData[i][11] = (uint8_t)((weight >> 16) & 0xFF);                                                             // WEIGHT  0xA1B2C3D4 -> 0xA1B2 -> 0xB2
        sensorData[i][12] = (uint8_t)((weight >> 8) & 0xFF);                                                              // WEIGHT  0xA1B2C3D4 -> 0xA1B2C3 -> 0xC3
        sensorData[i][13] = (uint8_t)(weight & 0xFF);                                                                     // WEIGHT  0xA1B2C3D4 -> 0xA1B2C3D4 -> 0xD4
        sensorData[i][14] = (uint8_t)(((uint32_t)(sensorsService.getSensors()[i].hx711->getWeight() * 100)) % 100);       // WEIGHT

        sensorData[i][15] = 29;                                                                                           // temp2
        sensorData[i][16] = 10;                                                                                           // temp2
        sensorData[i][17] = 50;                                                                                           // HUMEDITY
        sensorData[i][18] = 25;                                                                                           // HUMEDITY
        if (deviceIsMaster)
        {
            sensorData[i][19] = modemService.getBatteryLevel(); // BATTERY LEVEL 0 - 100
        }
        else
        {
            sensorData[i][19] = getSlaveBatteryLevel();
        }
    } else {
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
    nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, 1);
    nRF24L01p.startListening();
}

void printData(uint8_t *dataToPrint) {
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
    printf("=====================================================================================\r\n");
    printf("Bees Scale Device powered by Eugen Scobich\r\n");
    printf("Device Address: 0x%0X%0X%0X%0X%0X\r\n", deviceAddress[0], deviceAddress[1], deviceAddress[2], deviceAddress[3], deviceAddress[4]);
    printf("Device Mode: %s\r\n", deviceIsMaster ? "Master" : "Slave");
    printf("Waked up from: %s\r\n", deviceWasWakedUpFromStandby ? "Standby" : deviceWasWakedUpFromPower ? "Power On"
                                                                                                        : "Reset");
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();

    printf("Device Date: %02d.%02d.%02d\r\n", localDate.Date, localDate.Month, localDate.Year);
    printf("Device Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
    printf("=====================================================================================\r\n");
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    printf("Alarm!!!\r\n");
}

void goToStandByMode()
{
    printf("Go to StandBy Mode\r\n");
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