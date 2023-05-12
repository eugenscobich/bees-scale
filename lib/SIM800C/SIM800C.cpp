#include "SIM800C.h"
#include <stdio.h>
#include <cstring>

SIM800C::SIM800C(UART_HandleTypeDef* _huart, GPIO_TypeDef* _SIM800C_PWR_GPIOx, uint16_t _SIM800C_PWR_GPIO_Pin, GPIO_TypeDef* _SIM800C_DTR_GPIOx, uint16_t _SIM800C_DTR_GPIO_Pin):
    huart(_huart),
    SIM800C_PWR_GPIOx(_SIM800C_PWR_GPIOx),
    SIM800C_PWR_GPIO_Pin(_SIM800C_PWR_GPIO_Pin),
    SIM800C_DTR_GPIOx(_SIM800C_DTR_GPIOx),
    SIM800C_DTR_GPIO_Pin(_SIM800C_DTR_GPIO_Pin)
{
    sim800cState = {0};
}

void SIM800C::setState(uint8_t newState) {
    sim800cState.previousState = sim800cState.currentState;
    sim800cState.currentState = newState;
}

void SIM800C::txCpltCallback() {
    setState(SIM800C_STATE_TX_COMPLETED);
}

void SIM800C::rxCpltCallback() {
    setState(SIM800C_STATE_RX_COMPLETED);
}

SIM800CState SIM800C::update() {
    uint32_t currentHalTick = HAL_GetTick();
    if (sim800cState.currentState != sim800cState.previousState) {
        sim800cState.previousState = sim800cState.currentState;
        if (sim800cState.currentState == SIM800C_STATE_TX_COMPLETED) {
            printf("SIM800C: TX Completed (Command sent: %s)\r\n", sim800cState.cmd);
            sim800cState.currentState = SIM800C_STATE_WAITING_RESPONSE;
        }

        if (sim800cState.currentState == SIM800C_STATE_RX_COMPLETED) {
            printf("SIM800C: RX Completed\r\n");
            printf("SIM800C: %s", (char *)sim800cState.rxBuffer);
            // check pattern
            if(strstr((char *)sim800cState.rxBuffer, expectedPattern)) {
                setState(SIM800C_STATE_PATTERN_MATCHED);
            } else {
                setState(SIM800C_STATE_ERROR);
            }
        }
        if (sim800cState.currentState == SIM800C_STATE_PATTERN_MATCHED) {
            printf("SIM800C: Patern Matched\r\n");
        }
    }

    if (startReceiveTick > 0 && startReceiveTick + receiveTimeoutInTicks < currentHalTick) {
        printf("SIM800C: RX Timout\r\n");
        printf("SIM800C: %s", (char *)sim800cState.rxBuffer);
        // check pattern
        if(strstr((char *)sim800cState.rxBuffer, expectedPattern)) {
            setState(SIM800C_STATE_PATTERN_MATCHED);
        } else {
            setState(SIM800C_STATE_ERROR);
        }
    }
    return sim800cState;
}

void SIM800C::sendCmdAndSetExpectedPatternAndTimeout(char* cmd, char* _expectedPattern, uint16_t timeout) {
    sim800cState.cmd = cmd;
    sim800cState.currentState = SIM800C_STATE_RUNNING;
    sim800cState.previousState = SIM800C_STATE_UNKNOWN;
    expectedPattern = _expectedPattern;
    receiveTimeoutInTicks = timeout;
    startReceiveTick = HAL_GetTick();
    memset(sim800cState.rxBuffer, 0, sizeof(sim800cState.rxBuffer));
    HAL_UART_Transmit_IT(huart, (uint8_t*)cmd, strlen(cmd));
    HAL_UART_Receive_IT(huart, sim800cState.rxBuffer, sizeof(sim800cState.rxBuffer));
}