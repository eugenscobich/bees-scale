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
void readSensorAndPopulateData(uint8_t i);
void sendData(uint8_t i);
uint8_t getSlaveBatteryLevel();

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

    
        // ================ Sandbox
        if (deviceIsMaster)
        {
            nRF24L01p.init();
            nRF24L01p.enablePayloadWithAknoladge();
            nRF24L01p.enableDynamicPayload(0);
            nRF24L01p.enableDynamicPayload(1);
            nRF24L01p.stopListening();
            //nRF24L01p.openReadingPipe(NRF_MASTER_ADDRESS, 1);
            nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS);
            data[0] = 0x00;
            data[1] = 0x01;
            data[2] = 0x01;
            data[3] = 0x00;
            data[4] = 0x00;
            data[5] = 0x04;
            data[6] = 0x0D;
            data[7] = 0x00;
            data[8] = 0x04;
            data[9] = 0x2B;
            for (uint8_t i = 10; i < 32; i++) {
                data[i] = 0;
            }
            printf("Send Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);
            bool successSendCmd = nRF24L01p.write(data);
            if (successSendCmd)
            {
                if (nRF24L01p.isDataAvailable())
                {
                    nRF24L01p.receive(data);
                    printf("Recieved Sensor Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                               data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15],
                               data[16], data[17], data[18], data[19]);
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

            data[0] = 0x00;
            data[1] = 0x01;
            data[2] = 0x01;
            data[3] = 0x00;
            data[4] = 0x00;
            data[5] = 0x04;
            data[6] = 0x0D;
            data[7] = 0x00;
            data[8] = 0x04;
            data[9] = 0x2B;
            for (uint8_t i = 10; i < 32; i++) {
                data[i] = 0;
            }

            successSendCmd = nRF24L01p.write(data);
            nRF24L01p.writeTxFifo(data);
            if (successSendCmd)
            {
                if (nRF24L01p.isDataAvailable())
                {
                    nRF24L01p.receive(data);
                    printf("Recieved Sensor Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                               data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15],
                               data[16], data[17], data[18], data[19]);
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
            
            data[0] = 0x00;
            data[1] = 0x01;
            data[2] = 0x01;
            data[3] = 0x00;
            data[4] = 0x00;
            data[5] = 0x04;
            data[6] = 0x0D;
            data[7] = 0x00;
            data[8] = 0x04;
            data[9] = 0x2B;
            for (uint8_t i = 10; i < 32; i++) {
                data[i] = 0;
            }
            successSendCmd = nRF24L01p.write(data);
            if (successSendCmd)
            {
                if (nRF24L01p.isDataAvailable())
                {
                    nRF24L01p.receive(data);
                    printf("Recieved Sensor Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                               data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15],
                               data[16], data[17], data[18], data[19]);
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

            //nRF24L01p.printAllRegisters();
            nRF24L01p.powerDown();
            goToStandByMode();
        }
        else
        {
            nRF24L01p.isPowerUp();
            if (nRF24L01p.isPowerUp() && nRF24L01p.isInRxMode())
            {
                if (nRF24L01p.isDataReceived()) {
                    nRF24L01p.receive(data);
                    printf("Recieved Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);

                    startDelayTick = HAL_GetTick();
                    while (true)
                    {
                        if (nRF24L01p.isDataReceived()) {
                            nRF24L01p.receive(data);
                            printf("Recieved Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                                data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);
                        } else if (startDelayTick + 100 < HAL_GetTick())
                        {
                            printf("Timeout\r\n");
                            break;
                        }
                    }
                    nRF24L01p.powerDown();
                    goToStandByMode();
                } else {
                    printf("NRF is UP and RX but no data\r\n");
                }
            }
            else
            {
                printf("Init NRF\r\n");
                nRF24L01p.init();
                nRF24L01p.enablePayloadWithAknoladge();
                nRF24L01p.enableDynamicPayload(0);
                nRF24L01p.enableDynamicPayload(1);
                nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, 1);

                nRF24L01p.flushTx();
                //0x0000000000000000000000000000001D0A3219EA
                uint8_t data1[32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x0A, 0x32, 0x19, 0xEA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
                printf("Write Acknowledge Payload: %d\r\n", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, data1, 32));
                printf("Write Acknowledge Payload: %d\r\n", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, data1, 32));
                //0x28F05EB50500007E1B060000ACDE461D0A3219EA
                uint8_t data2[32] = {0x28, 0xF0, 0x5E, 0xB5, 0x05, 0x00, 0x00, 0x7E, 0x1B, 0x06, 0x00, 0x00, 0xAC, 0xDE, 0x46, 0x1D, 0x0A, 0x32, 0x19, 0xEA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
                printf("Write Acknowledge Payload: %d\r\n", nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, data2, 32));

                nRF24L01p.startListening();

                //nRF24L01p.printAllRegisters();
            }
        }
        goToStandByMode();
        // =====================================  Sandbox

        

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
                    nRF24L01p.receive(data);
                    printf("Recieved Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);

                    startDelayTick = HAL_GetTick();
                    while (true)
                    {
                        if (nRF24L01p.isDataReceived())
                        {
                            nRF24L01p.receive(data);
                            printf("Recieved Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);
                        }
                        else if (startDelayTick + 100 < HAL_GetTick())
                        {
                            printf("Timeout\r\n");
                            break;
                        }
                    }
                    timeService.setDateAndTime(data);
                    timeService.setAlarmTime(data);
                    nRF24L01p.powerDown();
                    goToStandByMode();
                } else {
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

                readSensorAndPopulateData(0);
                readSensorAndPopulateData(1);
                readSensorAndPopulateData(2);

                modemResultStatus = modemService.sendData(sensorData);
                printf("Master data was sent successful\r\n");

                printf("Ask Slave 1, Sensor one details.\r\n");
                bool slave1IsPrezent = true;
                printf("Init NRF\r\n");
                nRF24L01p.init();
                nRF24L01p.enablePayloadWithAknoladge();
                nRF24L01p.enableDynamicPayload(0);
                nRF24L01p.enableDynamicPayload(1);
                nRF24L01p.stopListening();
                nRF24L01p.openWritingPipe(NRF_SLAVE_ADDRESS);

                timeService.populateDataWithDateAndTime(data);
                timeService.populateDataWithAlarmTimeFor(data, 30, SECONDS);
                printf("Send Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);

                bool successSendCmd = nRF24L01p.write(data);
                if (successSendCmd)
                {
                    if (nRF24L01p.isDataAvailable())
                    {
                        nRF24L01p.receive(sensorData[0]);
                        printf("Recieved Sensor Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               sensorData[0][0], sensorData[0][1], sensorData[0][2], sensorData[0][3], sensorData[0][4], sensorData[0][5], sensorData[0][6], sensorData[0][7],
                               sensorData[0][8], sensorData[0][9], sensorData[0][10], sensorData[0][11], sensorData[0][12], sensorData[0][13], sensorData[0][14], sensorData[0][15],
                               sensorData[0][16], sensorData[0][17], sensorData[0][18], sensorData[0][19]);
                    }
                    else
                    {
                        printf("No Data\r\n");
                        slave1IsPrezent = false;
                    }
                }
                else
                {
                    printf("Send CMD failed: 0x%02X\r\n", data[0]);
                }

                successSendCmd = nRF24L01p.write(data);
                nRF24L01p.writeTxFifo(data);
                if (successSendCmd)
                {
                    if (nRF24L01p.isDataAvailable())
                    {
                        nRF24L01p.receive(sensorData[1]);
                        printf("Recieved Sensor Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               sensorData[1][0], sensorData[1][1], sensorData[1][2], sensorData[1][3], sensorData[1][4], sensorData[1][5], sensorData[1][6], sensorData[1][7],
                               sensorData[1][8], sensorData[1][9], sensorData[1][10], sensorData[1][11], sensorData[1][12], sensorData[1][13], sensorData[1][14], sensorData[1][15],
                               sensorData[1][16], sensorData[1][17], sensorData[1][18], sensorData[1][19]);
                    }
                    else
                    {
                        printf("No Data\r\n");
                        slave1IsPrezent = false;
                    }
                }
                else
                {
                    printf("Send CMD failed: 0x%02X\r\n", data[0]);
                }

                successSendCmd = nRF24L01p.write(data);
                if (successSendCmd)
                {
                    if (nRF24L01p.isDataAvailable())
                    {
                        nRF24L01p.receive(sensorData[2]);
                        printf("Recieved Sensor Data: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               sensorData[2][0], sensorData[2][1], sensorData[2][2], sensorData[2][3], sensorData[2][4], sensorData[2][5], sensorData[2][6], sensorData[2][7],
                               sensorData[2][8], sensorData[2][9], sensorData[2][10], sensorData[2][11], sensorData[2][12], sensorData[2][13], sensorData[2][14], sensorData[2][15],
                               sensorData[2][16], sensorData[2][17], sensorData[2][18], sensorData[2][19]);
                    }
                    else
                    {
                        printf("No Data\r\n");
                        slave1IsPrezent = false;
                    }
                }
                else
                {
                    printf("Send CMD failed: 0x%02X\r\n", data[0]);
                }
                nRF24L01p.powerDown();

                if (slave1IsPrezent)
                {
                    modemResultStatus = modemService.sendData(sensorData);
                    printf("Slave data was sent successful\r\n");
                }
                else
                {
                    printf("Slave data is not sent\r\n");
                }

                modemService.powerDown();
                timeService.setAlarmFor(30, SECONDS);
                goToStandByMode();
            }
            else
            {
                printf("We are slave, read sensors, enable radio, set ack packages and go to sleep to wait commands!\r\n");
                readSensorAndPopulateData(0);
                readSensorAndPopulateData(1);
                readSensorAndPopulateData(2);

                printf("Init NRF\r\n");
                nRF24L01p.init();
                nRF24L01p.enablePayloadWithAknoladge();
                nRF24L01p.enableDynamicPayload(0);
                nRF24L01p.enableDynamicPayload(1);
                nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, 1);

                nRF24L01p.flushTx();
                printf("Write Acknowledge Payload: %d (0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)\r\n",
                       nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[0], 32),
                       sensorData[0][0], sensorData[0][1], sensorData[0][2], sensorData[0][3], sensorData[0][4], sensorData[0][5], sensorData[0][6], sensorData[0][7],
                       sensorData[0][8], sensorData[0][9], sensorData[0][10], sensorData[0][11], sensorData[0][12], sensorData[0][13], sensorData[0][14], sensorData[0][15],
                       sensorData[0][16], sensorData[2][17], sensorData[2][18], sensorData[2][19]);
                printf("Write Acknowledge Payload: %d (0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)\r\n",
                       nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[1], 32),
                       sensorData[1][0], sensorData[1][1], sensorData[1][2], sensorData[1][3], sensorData[1][4], sensorData[1][5], sensorData[1][6], sensorData[1][7],
                       sensorData[1][8], sensorData[1][9], sensorData[1][10], sensorData[1][11], sensorData[1][12], sensorData[1][13], sensorData[1][14], sensorData[1][15],
                       sensorData[1][16], sensorData[2][17], sensorData[2][18], sensorData[2][19]);
                printf("Write Acknowledge Payload: %d (0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)\r\n",
                       nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[2], 32),
                       sensorData[2][0], sensorData[2][1], sensorData[2][2], sensorData[2][3], sensorData[2][4], sensorData[2][5], sensorData[2][6], sensorData[2][7],
                       sensorData[2][8], sensorData[2][9], sensorData[2][10], sensorData[2][11], sensorData[2][12], sensorData[2][13], sensorData[2][14], sensorData[2][15],
                       sensorData[2][16], sensorData[2][17], sensorData[2][18], sensorData[2][19]);
                nRF24L01p.startListening();

                goToStandByMode();
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

            timeService.setAlarmFor(20, SECONDS);
            goToStandByMode();
        }
        else
        {
            printf("We are slave, read sensors, enable radio, set ack packages and go to sleep to wait commands!\r\n");
            readSensorAndPopulateData(0);
            readSensorAndPopulateData(1);
            readSensorAndPopulateData(2);

            printf("Init NRF\r\n");
            nRF24L01p.init();
            nRF24L01p.enablePayloadWithAknoladge();
            nRF24L01p.enableDynamicPayload(0);
            nRF24L01p.enableDynamicPayload(1);
            nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, 1);

            nRF24L01p.flushTx();
            printf("Write Acknowledge Payload: %d (0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)\r\n",
                   nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[0], 32),
                   sensorData[0][0], sensorData[0][1], sensorData[0][2], sensorData[0][3], sensorData[0][4], sensorData[0][5], sensorData[0][6], sensorData[0][7],
                   sensorData[0][8], sensorData[0][9], sensorData[0][10], sensorData[0][11], sensorData[0][12], sensorData[0][13], sensorData[0][14], sensorData[0][15],
                   sensorData[0][16], sensorData[2][17], sensorData[2][18], sensorData[2][19]);
            printf("Write Acknowledge Payload: %d (0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)\r\n",
                   nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[1], 32),
                   sensorData[1][0], sensorData[1][1], sensorData[1][2], sensorData[1][3], sensorData[1][4], sensorData[1][5], sensorData[1][6], sensorData[1][7],
                   sensorData[1][8], sensorData[1][9], sensorData[1][10], sensorData[1][11], sensorData[1][12], sensorData[1][13], sensorData[1][14], sensorData[1][15],
                   sensorData[1][16], sensorData[2][17], sensorData[2][18], sensorData[2][19]);
            printf("Write Acknowledge Payload: %d (0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)\r\n",
                   nRF24L01p.writeAcknowledgePayload(RX_PIPE_1, sensorData[2], 32),
                   sensorData[2][0], sensorData[2][1], sensorData[2][2], sensorData[2][3], sensorData[2][4], sensorData[2][5], sensorData[2][6], sensorData[2][7],
                   sensorData[2][8], sensorData[2][9], sensorData[2][10], sensorData[2][11], sensorData[2][12], sensorData[2][13], sensorData[2][14], sensorData[2][15],
                   sensorData[2][16], sensorData[2][17], sensorData[2][18], sensorData[2][19]);
            nRF24L01p.startListening();

            goToStandByMode();
        }
    }

    while (1)
    {
        update();
    }
}

uint8_t getSlaveBatteryLevel()
{

    HAL_GPIO_WritePin(BATERY_LEVEL_CHECK_GPIO_Port, BATERY_LEVEL_CHECK_Pin, GPIO_PIN_RESET);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 1000);   // poll for conversion
    uint32_t value = HAL_ADC_GetValue(&hadc1); // get the adc value
    HAL_ADC_Stop(&hadc1);                      // stop adc
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
    return (uint8_t)((value - 2643) / 832);
}

void readSensorAndPopulateData(uint8_t i)
{
    printf("Populate sensor %d data\r\n", i);
    sensorsService.readSensors(i);
    for (uint8_t j = 0; j < 8; j++)
    {
        sensorData[i][j] = sensorsService.getSensors()[i].ds18b20->getRom()[j]; // ROM
    }
    sensorData[i][8] = (uint8_t)(sensorsService.getSensors()[i].ds18b20->getTemperature());                           // TEMP_OUT
    sensorData[i][9] = (uint8_t)(((uint16_t)(sensorsService.getSensors()[i].ds18b20->getTemperature() * 100)) % 100); // TEMP_OUT
    sensorData[i][10] = (uint8_t)(((uint32_t)(sensorsService.getSensors()[i].hx711->getWeight())) >> 24);             // WEIGHT
    sensorData[i][11] = (uint8_t)((((uint32_t)(sensorsService.getSensors()[i].hx711->getWeight())) >> 16) & 0xFF);    // WEIGHT
    sensorData[i][12] = (uint8_t)((((uint32_t)(sensorsService.getSensors()[i].hx711->getWeight())) >> 8) & 0xFF);     // WEIGHT
    sensorData[i][13] = (uint8_t)(((uint32_t)(sensorsService.getSensors()[i].hx711->getWeight())) & 0xFF);            // WEIGHT
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
}

void sendData(uint8_t i)
{
    nRF24L01p.stopListening();
    nRF24L01p.openWritingPipe(NRF_MASTER_ADDRESS);
    nRF24L01p.write(sensorData[i]);
    nRF24L01p.openReadingPipe(NRF_SLAVE_ADDRESS, 1);
    nRF24L01p.startListening();
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