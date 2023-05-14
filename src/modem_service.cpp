#include "modem_service.h"
#include <cstring>

ModemService::ModemService(SIM800C* _sim800c) :
    sim800c(_sim800c) {
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
        return MODEM_SUCCESS;
    } else {
        printf("Modem didn't respond. Power On!\r\n");
        HAL_Delay(2000);
        HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_RESET);
        HAL_Delay(4000);
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
    sim800cResult = sim800c->sendCmd("AT+CCID", "OK");
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
    SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBufferAndParseToInt("+CSQ: ", ",");
    if (findResult->found) {
        signalQuality = (uint8_t)findResult->valueInt;
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
    findResult = sim800c->findInRxBufferAndParseToInt("+CBC: ", ",", ",", "\r\n");
    if (findResult->found) {
        printf("Is Battery charging?: %d\r\n", (uint8_t)findResult->valueInt);
    } else {
        printf("Wasn't able to find if battery is chraging in the response\r\n");
        return MODEM_ERROR;
    }
    if (findResult->secondFound) {
        batteryLevel = (uint8_t)findResult->secondValueInt;
        printf("Battery level: %d\r\n", batteryLevel);
    } else {
        printf("Wasn't able to find battery level in the response\r\n");
        return MODEM_ERROR;
    }
    if (findResult->thirdFound) {
        batteryVoltage = (uint16_t)findResult->thirdValueInt;
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



void ModemService::disablePowerOnPin() {
    HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_SET);
}

ModemServiceResultStatus ModemService::configureModem() {
    printf("Configure SMS to work in text mode\r\n");
    sim800cResult = sim800c->sendCmd("AT+CMGF=1", "OK");



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
        if (sim800cResult->status != SIM800C_SUCCESS) {
            return MODEM_ERROR;
        }

        SIM800CFindInRxBufferResult* findResult = sim800c->findInRxBuffer("+CMGR: \"", "\",\"", "\",\"", "\",\"", "\"\r\n", "\r\n\r\nOK");
        if (findResult->secondFound) {
            printf("Phone number: %.*s\r\n", findResult->secondLength, findResult->secondValue);
        } else {
            printf("Wasn't able to find SMS #%d phone number\r\n", i);
            return MODEM_ERROR;
        }

        if (findResult->fifthFound) {
            printf("SMS message: %.*s\r\n", findResult->fifthLength, findResult->fifthValue);
        } else {
            printf("Wasn't able to find SMS #%d message\r\n", i);
            return MODEM_ERROR;
        }
    }


    return MODEM_SUCCESS;
}

ModemServiceResultStatus ModemService::deleteAllSMS() {
    printf("Delete All SMS\r\n");
    sim800cResult = sim800c->sendCmd("AT+CMGD=0,4", "OK", 25000);
    if (sim800cResult->status != SIM800C_SUCCESS) {
        return MODEM_ERROR;
    }
    return MODEM_SUCCESS;
}