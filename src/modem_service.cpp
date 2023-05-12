#include "modem_service.h"
#include "modem_state.h"

ModemService::ModemService(SIM800C* _sim800c) :
    sim800c(_sim800c) {
    modemState = {0};
}


bool ModemService::isModemPresent() {
    GPIO_PinState pinState = HAL_GPIO_ReadPin(SIM800C_PWR_GPIO_Port, SIM800C_PWR_Pin);
    return pinState == GPIO_PIN_SET;
}

void ModemService::update(State* state) {
    if (modemState.currentCmd != modemState.nextCmd) {
        modemState.previousCmd = modemState.currentCmd;
        modemState.currentCmd = modemState.nextCmd;
        modemState.nextCmd = MODEM_CMD_UNKNOWN;
        if (modemState.currentCmd == MODEM_CMD_CHECK_MODEM_AND_POWER_ON_IF_NEED) {
            sim800c->sendCmdAndSetExpectedPatternAndTimeout("AT\r\n", "OK'\r\n", 100);
        }
    }

    SIM800CState sim800cState = sim800c->update();


    if (modemState.currentState != modemState.previousState) {
        modemState.previousState = modemState.currentState;
        if (modemState.currentState == MODEM_STATE_SUCCESS) {
            state->currentState = STATE_MODEM_POWERED_ON;
        }
    }

}

void ModemService::checkModemAndPowerOnIfNeed() {
    modemState.nextCmd = MODEM_CMD_CHECK_MODEM_AND_POWER_ON_IF_NEED;
}