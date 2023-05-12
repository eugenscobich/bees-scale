#include "modem_service.h"

ModemService::ModemService(SIM800C* _sim800c) :
    sim800c(_sim800c) {
}

bool ModemService::isSIM800CPresent() {
    GPIO_PinState pinState = HAL_GPIO_ReadPin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin);
    return pinState == GPIO_PIN_SET;
}

ModemServiceResultStatus ModemService::startSIM800CIfNeed() {
    if (sim800c->sendCmd("AT\r\n", "OK\r\n", 100)->status == SIM800C_SUCCESS) {
        return MODEM_SUCCESS;
    } else {
        changeSim800CPwrPinToOuput();
        HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_RESET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_SET);
        return sim800c->sendCmd("AT\r\n", "OK\r\n", 100)->status == SIM800C_SUCCESS ? MODEM_SUCCESS : MODEM_ERROR;
    }
}


ModemServiceResultStatus ModemService::findSMSWithSettingsAndConfigureModem() {

}