#include "modem_service.h"

ModemService::ModemService(SIM800C* _sim800c) :
    sim800c(_sim800c) {
}

bool ModemService::isSIM800CPresent() {
    GPIO_PinState pinState = HAL_GPIO_ReadPin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin);
    return pinState == GPIO_PIN_SET;
}

ModemServiceResultStatus ModemService::startSIM800CIfNeed() {
    printf("Start SIM800C if need\r\n");
    SIM800CCmdResult* result = sim800c->sendCmd("AT\r\n", "OK\r\n", 1000, 3);
    if (result->status == SIM800C_SUCCESS) {
        return MODEM_SUCCESS;
    } else {
        printf("SIM800C didn't respond. Power On\r\n");
        HAL_Delay(2000);
        changeSim800CPwrPinToOuput();
        HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_RESET);
        //HAL_Delay(2000);
        //HAL_GPIO_WritePin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin, GPIO_PIN_SET);
        HAL_Delay(4000);
        result = sim800c->sendCmd("AT\r\n", "OK\r\n", 1000, 3);
        return result->status == SIM800C_SUCCESS ? MODEM_SUCCESS : MODEM_ERROR;
    }
}


ModemServiceResultStatus ModemService::findSMSWithSettingsAndConfigureModem() {

}