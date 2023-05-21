#include "modem_service.h"
#include "rtc.h"
#include <cstring>

ModemService::ModemService(SIM800C* _sim800c, void(*_updateFunction)()) :
    sim800c(_sim800c),
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
    printf("Start Modem if need\r\n");
    changeSim800CPwrPinToOuput();
    sim800cResult  = sim800c->sendCmd("AT", "OK", 1000, 2);
    if (sim800cResult->status == SIM800C_SUCCESS) {
        printf("Modem accepted AT command\r\n");
        return MODEM_SUCCESS;
    } else {
        printf("Modem didn't respond. Power On!\r\n");
        _nonBlockingDelay(2000);
        HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_RESET);
        _nonBlockingDelay(4000);
        printf("Check if Modem repond at AT command\r\n");
        sim800cResult = sim800c->sendCmd("AT", "OK", 2000, 2);
        if (sim800cResult->status != SIM800C_SUCCESS) {
            return MODEM_ERROR_IT_DIDN_T_REPONSD_AFTER_POWER_ON;
        }
        printf("Modem accepted AT command\r\n");
        return MODEM_SUCCESS;
    }
}

ModemServiceResultStatus ModemService::checkModemHealth() {
    sim800cResult = sim800c->sendCmd("AT", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    printf("Disable Modem echo\r\n");
    sim800cResult = sim800c->sendCmd("ATE0");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    printf("Enable full functionality of the modem\r\n");
    sim800cResult = sim800c->sendCmd("AT+CFUN=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    printf("Read SIM information to confirm whether the SIM is plugged.\r\n");
    sim800cResult = sim800c->sendCmd("AT+CCID", "OK", 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    printf("Check if the SIM is locked by pin.\r\n");
    sim800cResult = sim800c->sendCmd("AT+CPIN?", "+CPIN: READY", 5000);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    printf("Check signal quality\r\n");
    sim800cResult = sim800c->sendCmd("AT+CSQ", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(1, "+CSQ: ", ",");
    if (findResult->results[0].found) {
        signalQuality = (uint8_t)findResult->results[0].valueInt;
        printf("Signal quality: %d\r\n", signalQuality);
    } else {
        printf("Wasn't able to find signal quality in the response\r\n");
        return MODEM_ERROR;
    }

    printf("Check battery level\r\n");
    sim800cResult = sim800c->sendCmd("AT+CBC", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    findResult = sim800c->findInRxBuffer(3, "+CBC: ", ",", ",", "\r\n");
    if (findResult->results[0].found) {
        printf("Is Battery charging?: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        printf("Wasn't able to find if battery is chraging in the response\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[1].found) {
        batteryLevel = (uint8_t)findResult->results[1].valueInt;
        printf("Battery level: %d\r\n", batteryLevel);
    } else {
        printf("Wasn't able to find battery level in the response\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[2].found) {
        batteryVoltage = (uint16_t)findResult->results[2].valueInt;
        printf("Battery voltage: %d\r\n", batteryVoltage);
    } else {
        printf("Wasn't able to find battery voltage in the response\r\n");
        return MODEM_ERROR;
    }

    printf("Check whether it has registered in the network\r\n");
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
    printf("Configure SMS to work in text mode\r\n");
    sim800cResult = sim800c->sendCmd("AT+CMGF=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    printf("Decides how newly arrived SMS messages should be handled\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNMI=1,1,0,0,0", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
/*
    printf("Configure Date and Time\r\n");
    sim800cResult = sim800c->sendCmd("AT+CLTS?", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    uint8_t isRTCSyncEnabled;
    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBufferAndParseToInt("+CLTS: ", "\r\n");
    if (findResult->found) {
        isRTCSyncEnabled = (uint8_t)findResult->valueInt;
        printf("RTC Sync Enabled: %d\r\n", isRTCSyncEnabled);
        if (isRTCSyncEnabled == 0) {
            printf("Set Date and Time syncronisation\r\n");
            sim800cResult = sim800c->sendCmd("AT+CLTS=1", "OK");
            if (sim800cResult->status != SIM800C_SUCCESS) {
                return MODEM_ERROR;
            }

            sim800cResult = sim800c->sendCmd("AT+W", "OK");
            if (sim800cResult->status != SIM800C_SUCCESS) {
                return MODEM_ERROR;
            }

            enablePowerOnPin();
            _nonBlockingDelay(10000);
            HAL_NVIC_SystemReset();
        } else {
            sim800cResult = sim800c->sendCmd("AT+CCLK?", "OK");
            if (sim800cResult->status != SIM800C_SUCCESS) {
                return MODEM_ERROR;
            }
        }
    } else {
        printf("Wasn't able to find if RTC is enabled or not in modem\r\n");
        return MODEM_ERROR;
    }
*/

    // TODO can be disable reding the sms
/*
    printf("List of message storages\r\n");
    sim800cResult = sim800c->sendCmd("AT+CPMS?", "OK");
    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBufferAndParseToInt("+CPMS: \"SM_P\",", ",");
    if (findResult->found) {
        printf("Number Of SMS: %d\r\n", (uint8_t)findResult->valueInt);
    } else {
        printf("Wasn't able to find number of SMSs\r\n");
        return MODEM_ERROR;
    }
    for (size_t i = 1; i < findResult->valueInt + 1; i++) {
        printf("SMS #%d\r\n", i);
        char cmd[10];
        sprintf(cmd, "AT+CMGR=%d", i);
        sim800cResult = sim800c->sendCmd(cmd, "OK");
    }
*/

    return MODEM_SUCCESS;
}

ModemServiceResultStatus ModemService::findSMSWithSettingsAndConfigureModem() {
    printf("List of message storages\r\n");
    sim800cResult = sim800c->sendCmd("AT+CPMS?", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(1, "+CPMS: \"SM_P\",", ",");
    if (findResult->results[0].found) {
        printf("Number Of SMS: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        printf("Wasn't able to find number of SMSs\r\n");
        return MODEM_ERROR;
    }

    char* settingsSMSMessage;
    for (size_t i = 1; i < findResult->results[0].valueInt + 1; i++) {
        printf("SMS #%d\r\n", i);
        char cmd[10];
        sprintf(cmd, "AT+CMGR=%d", i);
        sim800cResult = sim800c->sendCmd(cmd, "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            return MODEM_ERROR;
        }

        SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(5, "+CMGR: \"", "\",\"", "\",\"", "\",\"", "\"\r\n", "\r\n\r\nOK");
        if (findResult->results[1].found) {
            printf("Phone number: %.*s\r\n", findResult->results[1].length, findResult->results[1].value);
        } else {
            printf("Wasn't able to find SMS #%d phone number\r\n", i);
            return MODEM_ERROR;
        }

        if (findResult->results[4].found) {
            printf("SMS message: %.*s\r\n", findResult->results[4].length, findResult->results[4].value);
        } else {
            printf("Wasn't able to find SMS #%d message\r\n", i);
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
            */
            const char* apnPrefix = "APN=\"";
            const char* apnStart = strstr(settingsSMSMessage + strlen(smsTitle), apnPrefix);
            const char* apnEnd = strstr(apnStart + strlen(apnPrefix), "\"");
            uint8_t apnLength = apnEnd - (apnStart + strlen(apnPrefix));
            strncpy(apn, apnStart + strlen(apnPrefix), apnLength > APN_MAX_LENGTH ? APN_MAX_LENGTH : apnLength);
            printf("Found APN:%s\r\n", apn);

            const char* apiKeyPrefix = "API_KEY=\"";
            const char* apiKeyStart = strstr(settingsSMSMessage + strlen(smsTitle), apiKeyPrefix);
            const char* apiKeyEnd = strstr(apiKeyStart + strlen(apiKeyPrefix), "\"");
            uint8_t apiKeyLength = apiKeyEnd - (apiKeyStart + strlen(apiKeyPrefix));
            strncpy(apiKey, apiKeyStart + strlen(apiKeyPrefix), apiKeyLength > API_KEY_MAX_LENGTH ? API_KEY_MAX_LENGTH : apiKeyLength);
            printf("Found API_KEY:%s\r\n", apiKey);

            const char* hostPrefix = "HOST=\"";
            const char* hostStart = strstr(settingsSMSMessage + strlen(smsTitle), hostPrefix);
            const char* hostEnd = strstr(hostStart + strlen(hostPrefix), "\"");
            uint8_t hostLength = hostEnd - (hostStart + strlen(hostPrefix));
            strncpy(host, hostStart + strlen(hostPrefix), hostLength > HOST_MAX_LENGTH ? HOST_MAX_LENGTH : hostLength);
            printf("Found HOST:%s\r\n", host);

            const char* userPrefix = "USER=\"";
            const char* userStart = strstr(settingsSMSMessage + strlen(smsTitle), userPrefix);
            if (userStart) {
                const char* userEnd = strstr(userStart + strlen(userPrefix), "\"");
                uint8_t userLength = userEnd - (userStart + strlen(userPrefix));
                strncpy(user, userStart + strlen(userPrefix), userLength > USER_MAX_LENGTH ? USER_MAX_LENGTH : userLength);
                printf("Found USER:%s\r\n", user);
            }

            const char* pwdPrefix = "PWD=\"";
            const char* pwdStart = strstr(settingsSMSMessage + strlen(smsTitle), pwdPrefix);
            if (pwdStart) {
                const char* pwdEnd = strstr(pwdStart + strlen(pwdPrefix), "\"");
                uint8_t pwdLength = pwdEnd - (pwdStart + strlen(pwdPrefix));
                strncpy(pwd, pwdStart + strlen(pwdPrefix), pwdLength > PWD_MAX_LENGTH ? PWD_MAX_LENGTH : pwdLength);
                printf("Found PWD:%s\r\n", pwd);
            }
            break;
        } else {
            printf("Found SMS is not about settings. It will be deleted!\r\n");
            char cmd[10];
            sprintf(cmd, "AT+CMGD=%d", i);
            sim800cResult = sim800c->sendCmd(cmd, "OK", 5000);
            if (sim800cResult->status != SIM800C_SUCCESS) {
                printf("Wasn't able to delete SMS with index: %d\r\n", i);
                return MODEM_ERROR;
            }
        }
    }

    if (settingsSMSMessage) {
        printf("Configure modem to be ready for GPRS\r\n");
        sim800cResult = sim800c->sendCmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            printf("Wasn't able to set the GPRS connection parameter\r\n");
            return MODEM_ERROR;
        }

        printf("Configure APN\r\n");
        // AT+SAPBR=3,1,APN,"APN"
        char apnCmd[APN_MAX_LENGTH + 20];
        sprintf(apnCmd, "AT+SAPBR=3,1,\"APN\",\"%s\"", apn);
        sim800cResult = sim800c->sendCmd(apnCmd, "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            printf("Wasn't able to set the APN parameter\r\n");
            return MODEM_ERROR;
        }

/*
        sim800cResult = sim800c->sendCmd("AT+CGATT?", "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            return MODEM_ERROR;
        }

        
        sim800cResult = sim800c->sendCmd("AT+CSTT=\"www\"", "OK");
        if (sim800cResult->status != SIM800C_SUCCESS) {
            return MODEM_ERROR;
        }
*/
        if (user[0] != '\0') {
            printf("Configure USER\r\n");
            // AT+SAPBR=3,1,USER,"Username"
            char userCmd[USER_MAX_LENGTH + 21];
            sprintf(userCmd, "AT+SAPBR=3,1,\"USER\",\"%s\"", user);
            sim800cResult = sim800c->sendCmd(userCmd, "OK");
            if (sim800cResult->status != SIM800C_SUCCESS) {
                printf("Wasn't able to set the USER parameter\r\n");
                return MODEM_ERROR;
            }
        }

        if (pwd[0] != '\0') {
            printf("Configure PWD\r\n");
            // AT+SAPBR=3,1,PWD,"Password"
            char pwdCmd[PWD_MAX_LENGTH + 20];
            sprintf(pwdCmd, "AT+SAPBR=3,1,\"PWD\",\"%s\"", pwd);
            sim800cResult = sim800c->sendCmd(pwdCmd, "OK");
            if (sim800cResult->status != SIM800C_SUCCESS) {
                printf("Wasn't able to set the PWD parameter\r\n");
                return MODEM_ERROR;
            }
        }
    } else {
        printf("Wasn't able to find settings SMS\r\n");
        return MODEM_ERROR_SETTINGS_SMS_WASN_T_FOUND;
    }
    return MODEM_SUCCESS;
}

ModemServiceResultStatus ModemService::deleteAllSMS() {
    printf("Delete All SMS\r\n");
    sim800cResult = sim800c->sendCmd("AT+CMGD=1,4", "OK", 25000);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    return MODEM_SUCCESS;
}



ModemServiceResultStatus ModemService::waitForSettingsSMS() {
    printf("Wait for settings SMS\r\n");
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    sim800cResult = sim800c->waitForMessage("+CMTI: ", 60000);
    if (sim800cResult->status == SIM800C_SUCCESS) {
        ModemServiceResultStatus modemServiceResultStatus = findSMSWithSettingsAndConfigureModem();
        if (modemServiceResultStatus == MODEM_SUCCESS) {
            return MODEM_SUCCESS;
        } else {
            printf("Received SMS doesn't conatins settings\r\n");
            return MODEM_ERROR_RECEIVED_SMS_DOESN_T_CONTAINS_SETTINGS;
        }
    } else if (sim800cResult->status == SIM800C_TIMEOUT) {
        printf("SMS is not received\r\n");
        return MODEM_ERROR_SMS_RECEIVED_TIMEOUT;
    }
    return MODEM_ERROR;
}


ModemServiceResultStatus ModemService::configureDateAndTime() {
    printf("Open GPRS context\r\n");

    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");

    sim800cResult = sim800c->sendCmd("AT+SAPBR=2,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    sim800cResult = sim800c->sendCmd("AT+SAPBR=1,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    printf("Set NTP Use bear profile 1\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTPCID=1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    printf("Set NTP service url and local time zone\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTP=\"time1.google.com\",0", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    printf("Start sync network time\r\n");
    sim800cResult = sim800c->sendCmd("AT+CNTP", "+CNTP: 1", 10000, 2);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        printf("Network time syncronisation is unsuccessful\r\n");
        return MODEM_ERROR;
    }

    printf("Network time syncronisation is successful\r\n");
    sim800cResult = sim800c->sendCmd("AT+CCLK?", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer(7, "+CCLK: \"", "/", "/", ",", ":", ":", "+", "\"");
    if (findResult->results[0].found) {
        printf("Found Date: %d\r\n", (uint8_t)findResult->results[0].valueInt);
    } else {
        printf("Wasn't able to find the date\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[1].found) {
        printf("Found Month: %d\r\n", (uint8_t)findResult->results[1].valueInt);
    } else {
        printf("Wasn't able to find the month\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[2].found) {
        printf("Found Year: %d\r\n", (uint8_t)findResult->results[2].valueInt);
    } else {
        printf("Wasn't able to find year\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[3].found) {
        printf("Found Hours: %d\r\n", (uint8_t)findResult->results[3].valueInt);
    } else {
        printf("Wasn't able to find hours\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[4].found) {
        printf("Found Minutes: %d\r\n", (uint8_t)findResult->results[4].valueInt);
    } else {
        printf("Wasn't able to find minutes\r\n");
        return MODEM_ERROR;
    }
    if (findResult->results[5].found) {
        printf("Found Seconds: %d\r\n", (uint8_t)findResult->results[5].valueInt);
    } else {
        printf("Wasn't able to find seconds\r\n");
        return MODEM_ERROR;
    }

    HAL_RTC_SetLocalDate((uint8_t)findResult->results[1].valueInt, (uint8_t)findResult->results[0].valueInt, (uint8_t)findResult->results[2].valueInt);
    HAL_RTC_SetLocalTime((uint8_t)findResult->results[3].valueInt, (uint8_t)findResult->results[4].valueInt, (uint8_t)findResult->results[5].valueInt);

    printf("Stop GPRS\r\n");
    sim800cResult = sim800c->sendCmd("AT+SAPBR=0,1", "OK");
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }

    return MODEM_SUCCESS;
}