#include "modem_service.h"
#include "rtc.h"
#include <stdio.h>
#include <cstring>
#include "led_service.h"
#include "error_codes.h"

#define CLASS_NAME "ModemServ"
#include "log.h"
                    
ModemService::ModemService(SIM800C* _sim800c, SensorsService* _sensorsService, void(*_updateFunction)(), void(*_errorFunction)(uint8_t)) :
    sim800c(_sim800c),
    sensorsService(_sensorsService),
    updateFunction(_updateFunction),
    errorFunction(_errorFunction)
{

}

void ModemService::nonBlockingDelay(uint32_t delayInTicks) {
    startDelayTick = HAL_GetTick();
    while (true) {
        if (startDelayTick + delayInTicks < HAL_GetTick()) {
            return;
        }
        updateFunction();
    }
}

bool ModemService::isModemPresent() {
    GPIO_PinState pinState = HAL_GPIO_ReadPin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin);
    return pinState == GPIO_PIN_SET;
}

void ModemService::startModemIfNeed() {
    logInfo("Start Modem if need\r\n");
    changeSim800CPwrPinToOuput();
    sim800cResult  = sim800c->sendCmd("AT", "OK", 1000, 2);
    if (sim800cResult->status == SIM800C_SUCCESS) {
        logInfo("Modem accepted AT command!\r\n");
    } else {
        logWarn("Modem didn't respond\r\n");
        logInfo("Power on the Modem\r\n");
        nonBlockingDelay(2000);
        HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_RESET);
        nonBlockingDelay(4000);
        logInfo("Check if Modem repond at AT command\r\n");
        sim800cResult = sim800c->sendCmd("AT", "OK", 1000, 3);
        if (sim800cResult->status != SIM800C_SUCCESS) {
            logError("Modem cannot accept AT command\r\n");
            errorFunction(MODEM_COULD_NOT_ACCEPT_AT_COMMAND_ERROR_CODE);
        }
        logInfo("Modem accepted AT command!\r\n");
    }
}

void ModemService::checkModemHealth() {
    sim800cResult = sim800c->sendCmd("AT", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Modem cannot accept AT command\r\n");
        errorFunction(MODEM_COULD_NOT_ACCEPT_AT_COMMAND_ERROR_CODE);
    }

    logInfo("Disable modem echo\r\n");
    sim800cResult = sim800c->sendCmd("ATE0");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not disable modem echo\r\n");
        errorFunction(COULD_NOT_DISABLE_MODEM_ECHO_ERROR_CODE);
    }

    logInfo("Enable full functionality of the modem\r\n");
    sim800cResult = sim800c->sendCmd("AT+CFUN=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not enable full functionality of the modem\r\n");
        errorFunction(COULD_NOT_ENABLE_FULL_FUNCTIONALITY_ERROR_CODE);
    }

    logInfo("Read SIM information to confirm whether the SIM is plugged\r\n");
    sim800cResult = sim800c->sendCmd("AT+CCID", "OK", 1000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not read SIM information\r\n");
        errorFunction(COULD_NOT_READ_SIM_INFORMATION_ERROR_CODE);
    }

    logInfo("Check if the SIM is locked by pin\r\n");
    sim800cResult = sim800c->sendCmd("AT+CPIN?", "+CPIN: READY", 5000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not check if the SIM is locked by pin\r\n");
        errorFunction(COULD_NOT_CHECK_IF_THE_SIM_IS_LOCKED_ERROR_CODE);
    }

    logInfo("Check whether it has registered in the network\r\n");
    sim800cResult = sim800c->sendCmd("AT+CREG?", "+CREG: 0,1", 10000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not check whether it has registered in the network\r\n");
        errorFunction(COULD_NOT_CHECK_IF_THE_MODEM_IS_REISTERED_ERROR_CODE);
    }

    logInfo("Check signal quality\r\n");
    sim800cResult = sim800c->sendCmd("AT+CSQ", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not check signal quality\r\n");
        errorFunction(COULD_NOT_CHECK_THE_SIGNAL_QUALITY_ERROR_CODE);
    }

    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(1, "+CSQ: ", ",");
    if (findResult->results[0].found) {
        signalQuality = (uint8_t)findResult->results[0].valueInt;
        logInfo("Signal quality: %d\r\n", signalQuality);
    } else {
        logError("Wasn't able to find signal quality in the response\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_SIGNAL_QUALITY_IN_THE_RESPONSE_ERROR_CODE);
    }

    logInfo("Check battery level\r\n");
    sim800cResult = sim800c->sendCmd("AT+CBC", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not check battery level\r\n");
        errorFunction(COULD_NOT_CHECK_BATTERY_LEVEL_ERROR_CODE);
    }
    findResult = sim800c->findInRxBuffer(3, "+CBC: ", ",", ",", "\r\n");
    if (findResult->results[0].found) {
        logInfo("Is Battery charging?: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        logError("Wasn't able to find if battery is chraging or not\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_IF_BATTERY_IS_CHRAGING_OR_NOT_ERROR_CODE);
    }
    if (findResult->results[1].found) {
        batteryLevel = (uint8_t)findResult->results[1].valueInt;
        logInfo("Battery level: %d\r\n", batteryLevel);
    } else {
        logError("Wasn't able to find battery level\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_BATTERY_LEVEL_ERROR_CODE);
    }
    if (findResult->results[2].found) {
        batteryVoltage = (uint16_t)findResult->results[2].valueInt;
        logInfo("Battery voltage: %d\r\n", batteryVoltage);
    } else {
        logError("Wasn't able to find battery voltage\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_BATTERY_VOLTAGE_ERROR_CODE);
    }

}

void ModemService::enablePowerOnPin() {
    HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_RESET);
}

void ModemService::disablePowerOnPin() {
    HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_SET);
}

void ModemService::configureModem() {
    logInfo("Configure SMS to work in text mode\r\n");
    sim800cResult = sim800c->sendCmd("AT+CMGF=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not configure SMS to work in text mode\r\n");
        errorFunction(COULD_NOT_CONFIGURE_SMS_TO_WORK_IN_TEXT_MODE_ERROR_CODE);
    }

    logInfo("Decides how newly arrived SMS messages should be handled\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNMI=1,1,0,0,0", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not decides how newly arrived SMS messages should be handled\r\n");
        errorFunction(COULD_NOT_DECIDES_HOW_NEWLY_ARRIVED_SMS_MESSAGES_SHOULD_BE_HANDLED_ERROR_CODE);
    }
}

void ModemService::findSMSWithSettingsAndConfigureModem() {
    logInfo("List of message storages\r\n");
    sim800cResult = sim800c->sendCmd("AT+CPMS?", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not list of message storages\r\n");
        errorFunction(COULD_NOT_LIST_OF_MESSAGE_STORAGES_ERROR_CODE);
    }
    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(1, "+CPMS: \"SM_P\",", ",");
    if (findResult->results[0].found) {
        logInfo("Number Of SMS: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        logError("Wasn't able to find number of SMSs\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_NUMBER_OF_SMS_ERROR_CODE);
    }
    foundSettingsSMS = false;
    char* settingsSMSMessage;
    for (size_t i = 1; i < findResult->results[0].valueInt + 1; i++) {
        logInfo("SMS #%d\r\n", i);
        char cmd[10];
        sprintf(cmd, "AT+CMGR=%d", i);
        sim800cResult = sim800c->sendCmd(cmd, "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            logError("Could not read SMS #%d\r\n", i);
            errorFunction(COULD_NOT_READ_SMS_ERROR_CODE);
        }

        SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(5, "+CMGR: \"", "\",\"", "\",\"", "\",\"", "\"\r\n", "\r\n\r\nOK");
        if (findResult->results[1].found) {
            strncpy(phoneNumber, findResult->results[1].value, findResult->results[1].length > PHONE_NUMBER_MAX_LENGTH ? PHONE_NUMBER_MAX_LENGTH : findResult->results[1].length);
            logInfo("Phone number: %s\r\n", phoneNumber);
        } else {
            logError("Wasn't able to find SMS #%d phone number\r\n", i);
            errorFunction(WASN_T_ABLE_TO_FIND_SMS_PHONE_NUMBER);
        }

        if (findResult->results[4].found) {
            logInfo("SMS message: %.*s\r\n", findResult->results[4].length, findResult->results[4].value);
        } else {
            logError("Wasn't able to find SMS #%d message\r\n", i);
            errorFunction(WASN_T_ABLE_TO_FIND_SMS_MESSAGE);
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
                errorFunction(WASN_T_ABLE_TO_DELETE_SMS_WITH_INDEX);
            }
        }
    }

    if (settingsSMSMessage) {
        logInfo("Configure modem to be ready for GPRS\r\n");
        sim800cResult = sim800c->sendCmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            logError("Wasn't able to set the GPRS connection parameter\r\n");
            errorFunction(WASN_T_ABLE_TO_SET_THE_GPRS_CONNECTION_PARAMETER);
        }

        logInfo("Configure APN\r\n");
        // AT+SAPBR=3,1,APN,"APN"
        char apnCmd[APN_MAX_LENGTH + 22];
        sprintf(apnCmd, "AT+SAPBR=3,1,\"APN\",\"%s\"", apn);
        sim800cResult = sim800c->sendCmd(apnCmd, "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            logError("Wasn't able to set the APN parameter\r\n");
            errorFunction(WASN_T_ABLE_TO_SET_THE_APN_PARAMETER);
        }

        if (user[0] != '\0') {
            logError("Configure USER\r\n");
            // AT+SAPBR=3,1,USER,"Username"
            char userCmd[USER_MAX_LENGTH + 23];
            sprintf(userCmd, "AT+SAPBR=3,1,\"USER\",\"%s\"", user);
            sim800cResult = sim800c->sendCmd(userCmd, "OK");
            if (sim800cResult->status != SIM800C_SUCCESS) {
                logError("Wasn't able to set the USER parameter\r\n");
                errorFunction(WASN_T_ABLE_TO_SET_THE_USER_PARAMETER);
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
                errorFunction(WASN_T_ABLE_TO_SET_THE_PWD_PARAMETER);
            }
        }
        foundSettingsSMS = true;
    } else {
        logError("Wasn't able to find settings SMS\r\n");
    }
}

void ModemService::deleteAllSMS() {
    logInfo("Delete All SMS\r\n");
    sim800cResult = sim800c->sendCmd("AT+CMGD=1,4", "OK", 25000);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not delete all SMS\r\n");
        errorFunction(COULD_NOT_DELETE_ALL_SMS);
    }
}

void ModemService::waitForSettingsSMS() {
    for (uint8_t i = 0; i < 2; i++)
    {
        logInfo("Wait for settings SMS\r\n");
        sim800cResult = sim800c->waitForMessage("+CMTI: ", 65535);
        if (sim800cResult->status == SIM800C_SUCCESS) {
            findSMSWithSettingsAndConfigureModem();
            if (foundSettingsSMS) {
                break;
            } else {
                logError("Received SMS doesn't conatins settings\r\n");
            }
        } else if (sim800cResult->status == SIM800C_TIMEOUT) {
            logError("SMS is not received in 65s\r\n");
        }
    }
}

void ModemService::openBearer() {
    logInfo("Close previous bearer\r\n");
    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");
    
    logInfo("Query bearer\r\n");
    sim800cResult = sim800c->sendCmd("AT+SAPBR=2,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not query bearer\r\n");
        errorFunction(COULD_NOT_QUERY_BEARER);
    }

    logInfo("Open bearer\r\n");
    sim800cResult = sim800c->sendCmd("AT+SAPBR=1,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not open bearer\r\n");
        errorFunction(COULD_NOT_OPEN_BEARER);
    }
}

void ModemService::configureDateAndTime() {
    logInfo("Configure Date and Time\r\n");
    
    openBearer();

    logInfo("Set NTP use bear profile 1\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTPCID=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not set NTP use bear profile 1\r\n");
        errorFunction(COULD_NOT_SET_NTP_USE_BEAR_PROFILE_1);
    }

    logInfo("Set NTP service url and local time zone\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTP=\"time1.google.com\",0", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not set NTP service url and local time zone\r\n");
        errorFunction(COULD_NOT_SET_NTP_SERVICE_URL_AND_LOCAL_TIME_ZONE);
    }

    logInfo("Start sync network time\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTP", "+CNTP: 1", 10000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Network time syncronisation is unsuccessful\r\n");
        errorFunction(NETWORK_TIME_SYNCRONISATION_IS_UNSUCCESSFUL);
    }

    logInfo("Network time syncronisation is successful\r\n");
    sim800cResult = sim800c->sendCmd("AT+CCLK?", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not sync the clock\r\n");
        errorFunction(COULD_NOT_SYNC_THE_CLOCK);
    }

    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(7, "+CCLK: \"", "/", "/", ",", ":", ":", "+", "\"");
    if (findResult->results[0].found) {
        logInfo("Found Date: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        logError("Wasn't able to find the date\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_THE_DATE);
    }
    if (findResult->results[1].found) {
        logInfo("Found Month: %d\r\n", (uint8_t)findResult->results[1].valueInt);
    } else {
        logError("Wasn't able to find the month\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_THE_MONTH);
    }
    if (findResult->results[2].found) {
        logInfo("Found Year: %d\r\n", (uint8_t)findResult->results[2].valueInt);
    } else {
        logError("Wasn't able to find year\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_YEAR);
    }
    if (findResult->results[3].found) {
        logInfo("Found Hours: %d\r\n", (uint8_t)findResult->results[3].valueInt);
    } else {
        logError("Wasn't able to find hours\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_HOURS);
    }
    if (findResult->results[4].found) {
        logInfo("Found Minutes: %d\r\n", (uint8_t)findResult->results[4].valueInt);
    } else {
        logError("Wasn't able to find minutes\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_MINUTES);
    }
    if (findResult->results[5].found) {
        logInfo("Found Seconds: %d\r\n", (uint8_t)findResult->results[5].valueInt);
    } else {
        logError("Wasn't able to find seconds\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_SECONDS);
    }

    HAL_RTC_SetLocalDate((uint8_t)findResult->results[1].valueInt, (uint8_t)findResult->results[0].valueInt, (uint8_t)findResult->results[2].valueInt);
    HAL_RTC_SetLocalTime((uint8_t)findResult->results[3].valueInt, (uint8_t)findResult->results[4].valueInt, (uint8_t)findResult->results[5].valueInt);

    logInfo("Stop GPRS\r\n");
    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not stop bearer\r\n");
        errorFunction(COULD_NOT_STOP_BEARER);
    }
}

void ModemService::changeSim800CPwrPinToOuput() {
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

void ModemService::sendData(uint8_t sensorData[][32]) {
    logInfo("Send Data\r\n");

    openBearer();

    logInfo("Initialize HTTP service\r\n");
    sim800cResult = sim800c->sendCmd("AT+HTTPINIT", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not initialize HTTP service\r\n");
        errorFunction(COULD_NOT_INITIALIZE_HTTP_SERVICE);
    }

    logInfo("Set HTTP parameters value: CID\r\n");
    sim800cResult = sim800c->sendCmd("AT+HTTPPARA=\"CID\",1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not set HTTP parameters value: CID\r\n");
        errorFunction(COULD_NOT_SET_HTTP_PARAMETERS_VALUE_CID);
    }

    logInfo("Set HTTP parameters value: URL\r\n");
    char buff[100];
    sprintf(buff, "AT+HTTPPARA=\"URL\",\"http://%s/api/v1/%s/telemetry\"", host, apiKey);
    sim800cResult = sim800c->sendCmd(buff, "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not set HTTP parameters value: URL\r\n");
        errorFunction(COULD_NOT_SET_HTTP_PARAMETERS_VALUE_URL);
    }

    logInfo("Set HTTP parameters value: CONTENT\r\n");
    sim800cResult = sim800c->sendCmd("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not set HTTP parameters value: CONTENT\r\n");
        errorFunction(COULD_NOT_SET_HTTP_PARAMETERS_VALUE_CONTENT);
    }

    char data[500] = {0};
    strcat(data, "[");
    for (uint8_t i = 0; i < 3; i++)
    {
        if(sensorData[i][0] > 0) {
            char sensorDataJson[200];
            sprintf(sensorDataJson, "{\"id\"=\"%02X%02X%02X%02X%02X%02X%02X%02X\",\"tempOutside\"=\"%d.%02d\",\"weight\"=\"%lu.%02d\",\"temp\"=\"%d.%02d\",\"humidity\"=\"%d.%02d\",\"bat\"=\"%d\"}",
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
    logInfo("Input HTTP data\r\n");
    sprintf(buff, "AT+HTTPDATA=%d,100000", strlen(data));
    sim800cResult = sim800c->sendCmd(buff, "DOWNLOAD");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not receive DOWNLOAD\r\n");
        errorFunction(COULD_NOT_RECEIVE_DOWNLOAD);
    }
    logInfo("Data: %s\r\n", data);
    sim800cResult = sim800c->sendCmd(data);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Error send data\r\n");
        errorFunction(ERROR_SEND_DATA);
    }
    logInfo("HTTP method action\r\n");
    sim800cResult = sim800c->sendCmd("AT+HTTPACTION=1", "+HTTPACTION: ", 10000, 0);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not HTTP method action\r\n");
        errorFunction(COULD_NOT_HTTP_METHOD_ACTION);
    }

    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(3, "+HTTPACTION: ", ",", ",", "\n");
    if (findResult->results[1].found) {
        logInfo("Response code: %d\r\n", (uint8_t)findResult->results[1].valueInt);
    } else {
        logError("Wasn't able to find response code\r\n");
        errorFunction(WASN_T_ABLE_TO_FIND_RESPONSE_CODE);
    }
    // TODO implement retry
    logInfo("Read the HTTP server response\r\n");
    sim800cResult = sim800c->sendCmd("AT+HTTPREAD", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not read the HTTP server response\r\n");
        errorFunction(COULD_NOT_READ_THE_HTTP_SERVER_RESPONSE);
    }

    logInfo("Terminate HTTP service\r\n");
    sim800cResult = sim800c->sendCmd("AT+HTTPTERM", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not terminate HTTP service\r\n");
        errorFunction(COULD_NOT_TERMINATE_HTTP_SERVICE);
    }
    logInfo("Close bearer\r\n");
    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");
     if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not close bearer\r\n");
        errorFunction(COULD_NOT_CLOSE_BEARER);
    }
}

void ModemService::powerDown() {
    logInfo("Power down the modem\r\n");
    sim800cResult = sim800c->sendCmd("AT+CPOWD=1", "NORMAL POWER DOWN");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        logError("Could not power down the modem\r\n");
        errorFunction(COULD_NOT_POWER_DOWN_THE_MODEM);
    }
}

bool ModemService::isSettingsSMSFound() {
    return foundSettingsSMS;
}