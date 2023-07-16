#include "modem_service.h"
#include "rtc.h"
#include <stdio.h>
#include <cstring>

#define CLASS_NAME "ModemServ"
#include "log.h"
                    
ModemService::ModemService(SIM800C* _sim800c, SensorsService* _sensorsService, void(*_updateFunction)()) :
    sim800c(_sim800c),
    sensorsService(_sensorsService),
    updateFunction(_updateFunction)
{

}

void ModemService::_nonBlockingDelay(uint32_t delayInTicks) {
    startDelayTick = HAL_GetTick();
    while (true) {
        if (startDelayTick + delayInTicks < HAL_GetTick()) {
            return;
        }
        updateFunction();
    }
}

bool ModemService::isSIM800CPresent() {
    GPIO_PinState pinState = HAL_GPIO_ReadPin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin);
    return pinState == GPIO_PIN_SET;
}

ModemServiceResultStatus ModemService::startModemIfNeed() {
    logInfo("Start Modem if need\r\n");
    _changeSim800CPwrPinToOuput();
    sim800cResult  = sim800c->sendCmd("AT", "OK", 1000, 2);
    if (sim800cResult->status == SIM800C_SUCCESS) {
        logInfo("Modem accepted AT command!\r\n");
        return MODEM_SUCCESS;
    } else {
        logWarn("Modem didn't respond\r\n");
        logInfo("Power on the Modem!\r\n");
        _nonBlockingDelay(2000);
        HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_RESET);
        _nonBlockingDelay(4000);
        logInfo("Check if Modem repond at AT command\r\n");
        sim800cResult = sim800c->sendCmd("AT", "OK", 1000, 3);
        if (sim800cResult->status != SIM800C_SUCCESS) {
            return MODEM_ERROR_IT_DIDN_T_REPONSD_AFTER_POWER_ON;
        }
        logInfo("Modem accepted AT command!\r\n");
        return MODEM_SUCCESS;
    }
}

ModemServiceResultStatus ModemService::checkModemHealth() {
    sim800cResult = sim800c->sendCmd("AT", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    logInfo("Disable Modem echo\r\n");
    sim800cResult = sim800c->sendCmd("ATE0");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    logInfo("Enable full functionality of the modem\r\n");
    sim800cResult = sim800c->sendCmd("AT+CFUN=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    logInfo("Read SIM information to confirm whether the SIM is plugged.\r\n");
    sim800cResult = sim800c->sendCmd("AT+CCID", "OK", 1000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    logInfo("Check if the SIM is locked by pin.\r\n");
    sim800cResult = sim800c->sendCmd("AT+CPIN?", "+CPIN: READY", 5000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    logInfo("Check signal quality\r\n");
    sim800cResult = sim800c->sendCmd("AT+CSQ", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(1, "+CSQ: ", ",");
    if (findResult->results[0].found) {
        signalQuality = (uint8_t)findResult->results[0].valueInt;
        logInfo("Signal quality: %d\r\n", signalQuality);
    } else {
        logError("Wasn't able to find signal quality in the response\r\n");
        return MODEM_ERROR;
    }

    logInfo("Check battery level\r\n");
    sim800cResult = sim800c->sendCmd("AT+CBC", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    findResult = sim800c->findInRxBuffer(3, "+CBC: ", ",", ",", "\r\n");
    if (findResult->results[0].found) {
        logInfo("Is Battery charging?: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        logError("Wasn't able to find if battery is chraging in the response\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[1].found) {
        batteryLevel = (uint8_t)findResult->results[1].valueInt;
        logInfo("Battery level: %d\r\n", batteryLevel);
    } else {
        logError("Wasn't able to find battery level in the response\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[2].found) {
        batteryVoltage = (uint16_t)findResult->results[2].valueInt;
        logInfo("Battery voltage: %d\r\n", batteryVoltage);
    } else {
        logError("Wasn't able to find battery voltage in the response\r\n");
        return MODEM_ERROR;
    }

    logInfo("Check whether it has registered in the network\r\n");
    sim800cResult = sim800c->sendCmd("AT+CREG?", "+CREG: 0,1", 10000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    return MODEM_SUCCESS;
}

void ModemService::enablePowerOnPin() {
    HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_RESET);
}

void ModemService::disablePowerOnPin() {
    HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_SET);
}

ModemServiceResultStatus ModemService::configureModem() {
    logInfo("Configure SMS to work in text mode\r\n");
    sim800cResult = sim800c->sendCmd("AT+CMGF=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    logInfo("Decides how newly arrived SMS messages should be handled\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNMI=1,1,0,0,0", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    return MODEM_SUCCESS;
}

ModemServiceResultStatus ModemService::findSMSWithSettingsAndConfigureModem() {
    logInfo("List of message storages\r\n");
    sim800cResult = sim800c->sendCmd("AT+CPMS?", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(1, "+CPMS: \"SM_P\",", ",");
    if (findResult->results[0].found) {
        logInfo("Number Of SMS: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        logError("Wasn't able to find number of SMSs\r\n");
        return MODEM_ERROR;
    }

    char* settingsSMSMessage;
    for (size_t i = 1; i < findResult->results[0].valueInt + 1; i++) {
        logInfo("SMS #%d\r\n", i);
        char cmd[10];
        sprintf(cmd, "AT+CMGR=%d", i);
        sim800cResult = sim800c->sendCmd(cmd, "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            return MODEM_ERROR;
        }

        SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(5, "+CMGR: \"", "\",\"", "\",\"", "\",\"", "\"\r\n", "\r\n\r\nOK");
        if (findResult->results[1].found) {
            strncpy(phoneNumber, findResult->results[1].value, findResult->results[1].length > PHONE_NUMBER_MAX_LENGTH ? PHONE_NUMBER_MAX_LENGTH : findResult->results[1].length);
            logInfo("Phone number: %s\r\n", phoneNumber);
        } else {
            logError("Wasn't able to find SMS #%d phone number\r\n", i);
            return MODEM_ERROR;
        }

        if (findResult->results[4].found) {
            logInfo("SMS message: %.*s\r\n", findResult->results[4].length, findResult->results[4].value);
        } else {
            logError("Wasn't able to find SMS #%d message\r\n", i);
            return MODEM_ERROR;
        }

        const char* smsTitle = "Bees Scale Settings";
        settingsSMSMessage = strstr(findResult->results[4].value, smsTitle);
        if (settingsSMSMessage) {
            /*
            Bees Scale Settings
            APN="orange.md"
            USER=""
            PWD=""
            API_KEY="sdfjs3k5464n352kl679518i91nl"
            HOST="google.com"
            REFRESH=30m
            */
            const char* apnPrefix = "APN=\"";
            const char* apnStart = strstr(settingsSMSMessage + strlen(smsTitle), apnPrefix);
            const char* apnEnd = strstr(apnStart + strlen(apnPrefix), "\"");
            uint8_t apnLength = apnEnd - (apnStart + strlen(apnPrefix));
            strncpy(apn, apnStart + strlen(apnPrefix), apnLength > APN_MAX_LENGTH ? APN_MAX_LENGTH : apnLength);
            logInfo("Found APN: %s\r\n", apn);

            const char* apiKeyPrefix = "API_KEY=\"";
            const char* apiKeyStart = strstr(settingsSMSMessage + strlen(smsTitle), apiKeyPrefix);
            const char* apiKeyEnd = strstr(apiKeyStart + strlen(apiKeyPrefix), "\"");
            uint8_t apiKeyLength = apiKeyEnd - (apiKeyStart + strlen(apiKeyPrefix));
            strncpy(apiKey, apiKeyStart + strlen(apiKeyPrefix), apiKeyLength > API_KEY_MAX_LENGTH ? API_KEY_MAX_LENGTH : apiKeyLength);
            logInfo("Found API_KEY: %s\r\n", apiKey);

            const char* hostPrefix = "HOST=\"";
            const char* hostStart = strstr(settingsSMSMessage + strlen(smsTitle), hostPrefix);
            const char* hostEnd = strstr(hostStart + strlen(hostPrefix), "\"");
            uint8_t hostLength = hostEnd - (hostStart + strlen(hostPrefix));
            strncpy(host, hostStart + strlen(hostPrefix), hostLength > HOST_MAX_LENGTH ? HOST_MAX_LENGTH : hostLength);
            logInfo("Found HOST: %s\r\n", host);

            const char* userPrefix = "USER=\"";
            const char* userStart = strstr(settingsSMSMessage + strlen(smsTitle), userPrefix);
            if (userStart) {
                const char* userEnd = strstr(userStart + strlen(userPrefix), "\"");
                uint8_t userLength = userEnd - (userStart + strlen(userPrefix));
                strncpy(user, userStart + strlen(userPrefix), userLength > USER_MAX_LENGTH ? USER_MAX_LENGTH : userLength);
                logError("Found USER: %s\r\n", user);
            }

            const char* pwdPrefix = "PWD=\"";
            const char* pwdStart = strstr(settingsSMSMessage + strlen(smsTitle), pwdPrefix);
            if (pwdStart) {
                const char* pwdEnd = strstr(pwdStart + strlen(pwdPrefix), "\"");
                uint8_t pwdLength = pwdEnd - (pwdStart + strlen(pwdPrefix));
                strncpy(pwd, pwdStart + strlen(pwdPrefix), pwdLength > PWD_MAX_LENGTH ? PWD_MAX_LENGTH : pwdLength);
                logInfo("Found PWD: %s\r\n", pwd);
            }

            const char* refreshPrefix = "REFRESH=\"";
            const char* refreshStart = strstr(settingsSMSMessage + strlen(smsTitle), refreshPrefix);
            if (refreshStart) {
                const char* refreshEnd = strstr(refreshStart + strlen(refreshPrefix), "m\"");
                uint8_t refreshLength = refreshEnd - (refreshStart + strlen(refreshPrefix));
                refreshIntervalInMinutes = sim800c->charArray2int(refreshStart, refreshLength);
                logInfo("Found REFRESH: %d min\r\n", refreshIntervalInMinutes);
            }
            break;
        } else {
            logInfo("Found SMS is not about settings. It will be deleted!\r\n");
            char cmd[10];
            sprintf(cmd, "AT+CMGD=%d", i);
            sim800cResult = sim800c->sendCmd(cmd, "OK", 5000);
            if (sim800cResult->status != SIM800C_SUCCESS) {
                logError("Wasn't able to delete SMS with index: %d\r\n", i);
                return MODEM_ERROR;
            }
        }
    }

    if (settingsSMSMessage) {
        logInfo("Configure modem to be ready for GPRS\r\n");
        sim800cResult = sim800c->sendCmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            logError("Wasn't able to set the GPRS connection parameter\r\n");
            return MODEM_ERROR;
        }

        logInfo("Configure APN\r\n");
        // AT+SAPBR=3,1,APN,"APN"
        char apnCmd[APN_MAX_LENGTH + 22];
        sprintf(apnCmd, "AT+SAPBR=3,1,\"APN\",\"%s\"", apn);
        sim800cResult = sim800c->sendCmd(apnCmd, "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            logError("Wasn't able to set the APN parameter\r\n");
            return MODEM_ERROR;
        }

        if (user[0] != '\0') {
            logError("Configure USER\r\n");
            // AT+SAPBR=3,1,USER,"Username"
            char userCmd[USER_MAX_LENGTH + 21];
            sprintf(userCmd, "AT+SAPBR=3,1,\"USER\",\"%s\"", user);
            sim800cResult = sim800c->sendCmd(userCmd, "OK");
            if (sim800cResult->status != SIM800C_SUCCESS) {
                logError("Wasn't able to set the USER parameter\r\n");
                return MODEM_ERROR;
            }
        }

        if (pwd[0] != '\0') {
            logInfo("Configure PWD\r\n");
            // AT+SAPBR=3,1,PWD,"Password"
            char pwdCmd[PWD_MAX_LENGTH + 22];
            sprintf(pwdCmd, "AT+SAPBR=3,1,\"PWD\",\"%s\"", pwd);
            sim800cResult = sim800c->sendCmd(pwdCmd, "OK");
            if (sim800cResult->status != SIM800C_SUCCESS) {
                logError("Wasn't able to set the PWD parameter\r\n");
                return MODEM_ERROR;
            }
        }
    } else {
        logError("Wasn't able to find settings SMS\r\n");
        return MODEM_ERROR_SETTINGS_SMS_WASN_T_FOUND;
    }
    return MODEM_SUCCESS;
}

ModemServiceResultStatus ModemService::deleteAllSMS() {
    logInfo("Delete All SMS\r\n");
    sim800cResult = sim800c->sendCmd("AT+CMGD=1,4", "OK", 25000);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    return MODEM_SUCCESS;
}



ModemServiceResultStatus ModemService::waitForSettingsSMS() {
    logInfo("Wait for settings SMS\r\n");
    sim800cResult = sim800c->waitForMessage("+CMTI: ", 65535);
    if (sim800cResult->status == SIM800C_SUCCESS) {
        ModemServiceResultStatus modemServiceResultStatus = findSMSWithSettingsAndConfigureModem();
        if (modemServiceResultStatus == MODEM_SUCCESS) {
            return MODEM_SUCCESS;
        } else {
            logError("Received SMS doesn't conatins settings\r\n");
            return waitForSettingsSMS();
        }
    } else if (sim800cResult->status == SIM800C_TIMEOUT) {
        logError("SMS is not received\r\n");
        return MODEM_ERROR_SMS_RECEIVED_TIMEOUT;
    }
    return MODEM_ERROR;
}


ModemServiceResultStatus ModemService::configureDateAndTime() {
    logInfo("Open GPRS context\r\n");

    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");

    sim800cResult = sim800c->sendCmd("AT+SAPBR=2,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+SAPBR=1,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    logInfo("Set NTP Use bear profile 1\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTPCID=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    logInfo("Set NTP service url and local time zone\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTP=\"time1.google.com\",0", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    logInfo("Start sync network time\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTP", "+CNTP: 1", 10000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Network time syncronisation is unsuccessful\r\n");
        return MODEM_ERROR;
    }

    logInfo("Network time syncronisation is successful\r\n");
    sim800cResult = sim800c->sendCmd("AT+CCLK?", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(7, "+CCLK: \"", "/", "/", ",", ":", ":", "+", "\"");
    if (findResult->results[0].found) {
        logInfo("Found Date: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        logError("Wasn't able to find the date\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[1].found) {
        logInfo("Found Month: %d\r\n", (uint8_t)findResult->results[1].valueInt);
    } else {
        logError("Wasn't able to find the month\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[2].found) {
        logInfo("Found Year: %d\r\n", (uint8_t)findResult->results[2].valueInt);
    } else {
        logError("Wasn't able to find year\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[3].found) {
        logInfo("Found Hours: %d\r\n", (uint8_t)findResult->results[3].valueInt);
    } else {
        logError("Wasn't able to find hours\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[4].found) {
        logInfo("Found Minutes: %d\r\n", (uint8_t)findResult->results[4].valueInt);
    } else {
        logError("Wasn't able to find minutes\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[5].found) {
        logInfo("Found Seconds: %d\r\n", (uint8_t)findResult->results[5].valueInt);
    } else {
        logError("Wasn't able to find seconds\r\n");
        return MODEM_ERROR;
    }

    HAL_RTC_SetLocalDate((uint8_t)findResult->results[1].valueInt, (uint8_t)findResult->results[0].valueInt, (uint8_t)findResult->results[2].valueInt);
    HAL_RTC_SetLocalTime((uint8_t)findResult->results[3].valueInt, (uint8_t)findResult->results[4].valueInt, (uint8_t)findResult->results[5].valueInt);

    logInfo("Stop GPRS\r\n");
    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    return MODEM_SUCCESS;
}

void ModemService::_changeSim800CPwrPinToOuput() {
    HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_SET);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /*Configure GPIO pins : PAPin PAPin */
    GPIO_InitStruct.Pin = SIM800C_PWR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SIM800C_PWR_GPIO_Port, &GPIO_InitStruct);
}

uint8_t ModemService::getBatteryLevel() {
    return batteryLevel;
}

uint16_t ModemService::getRefreshIntervalInMinutes() {
    return refreshIntervalInMinutes;
}



ModemServiceResultStatus ModemService::sendData(uint8_t sensorData[][32]) {
    logInfo("Open GPRS context\r\n");

    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");

    sim800cResult = sim800c->sendCmd("AT+SAPBR=2,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+SAPBR=1,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+HTTPINIT", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+HTTPPARA=\"CID\",1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    char buff[100];
    sprintf(buff, "AT+HTTPPARA=\"URL\",\"http://%s/api/v1/%s/telemetry\"", host, apiKey);
    sim800cResult = sim800c->sendCmd(buff, "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    char data[500] = {0};
    strcat(data, "[");
    for (uint8_t i = 0; i < 3; i++)
    {
        if(sensorData[i][0] > 0) {
            char sensorDataJson[200];
            sprintf(sensorDataJson, "{\"id\"=\"%02X%02X%02X%02X%02X%02X%02X%02X\",\"tempOutside\"=\"%d.%02d\",\"weight\"=\"%d.%02d\",\"temp\"=\"%d.%02d\",\"humidity\"=\"%d.%02d\",\"bat\"=\"%d\"}",
                sensorData[i][0],
                sensorData[i][1],
                sensorData[i][2],
                sensorData[i][3],
                sensorData[i][4],
                sensorData[i][5],
                sensorData[i][6],
                sensorData[i][7],
                //tempOutside
                sensorData[i][8],
                sensorData[i][9],
                //weight
                (uint32_t)(sensorData[i][10] << 24 | sensorData[i][11] << 16 | sensorData[i][12] << 8 | sensorData[i][13]),
                sensorData[i][14],
                //temp
                sensorData[i][15],
                sensorData[i][16],
                //humidity
                sensorData[i][17],
                sensorData[i][18],
                //battery
                sensorData[i][19]
            );
            logInfo("Sensor: %d JSON data: %s\r\n", i, sensorDataJson);
            strcat(data, sensorDataJson);
            if (i < 2) {
                strcat(data, ",");
            }
        } else {
            logInfo("Sensor: %d is not present\r\n", i);
        }
    }
    strcat(data, "]");
    
    sprintf(buff, "AT+HTTPDATA=%d,100000", strlen(data));
    sim800cResult = sim800c->sendCmd(buff, "DOWNLOAD");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    //logError("Data: %s\r\n", data);
    sim800cResult = sim800c->sendCmd(data);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+HTTPACTION=1", "+HTTPACTION: ", 10000, 0);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(3, "+HTTPACTION: ", ",", ",", "\n");
    if (findResult->results[1].found) {
        logInfo("Response code: %d\r\n", (uint8_t)findResult->results[1].valueInt);
    } else {
        logError("Wasn't able to find response code\r\n");
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+HTTPREAD", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+HTTPTERM", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
   
    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");
    return MODEM_SUCCESS;
}

ModemServiceResultStatus ModemService::powerDown() {
    sim800cResult = sim800c->sendCmd("AT+CPOWD=1", "NORMAL POWER DOWN");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    return MODEM_SUCCESS;
}