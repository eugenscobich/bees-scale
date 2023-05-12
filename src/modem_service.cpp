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
            modemState.previousState = modemState.currentState;
            modemState.currentState = MODEM_STATE_RUNNING;
        }
    }

    SIM800CState sim800cState = sim800c->update();

    if (modemState.currentState == MODEM_STATE_RUNNING) {
        if (sim800cState.currentState == SIM800C_STATE_PATTERN_MATCHED) {
            modemState.previousState = modemState.currentState;
            modemState.currentState = MODEM_STATE_SUCCESS;
        }

        if (sim800cState.currentState == SIM800C_STATE_ERROR) {

            if (numberOfRetries == 0) {
                state->currentState = STATE_MODEM_STATE_ERROR;
            } else {
                numberOfRetries--;
                modemState.nextCmd = modemState.currentCmd;
                modemState.currentCmd = modemState.previousCmd;
                modemState.previousCmd = MODEM_CMD_UNKNOWN;
            }



            modemState.previousState = modemState.currentState;
            modemState.currentState = MODEM_STATE_ERROR;
        }
    }


    if (modemState.currentState != modemState.previousState) {
        modemState.previousState = modemState.currentState;
        if (modemState.currentState == MODEM_STATE_SUCCESS) {
            state->currentState = STATE_MODEM_POWERED_ON;
        } 

        if (modemState.currentState == MODEM_STATE_ERROR) {
            if (numberOfRetries == 0) {
                state->currentState = STATE_MODEM_STATE_ERROR;
            } else {
                numberOfRetries--;
                modemState.nextCmd = modemState.currentCmd;
                modemState.currentCmd = modemState.previousCmd;
                modemState.previousCmd = MODEM_CMD_UNKNOWN;
            }
        } 
    }

}

void ModemService::checkModemAndPowerOnIfNeed() {
    modemState.nextCmd = MODEM_CMD_CHECK_MODEM_AND_POWER_ON_IF_NEED;
    numberOfRetries = 2;
}