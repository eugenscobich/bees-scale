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
    sim800cCmdResult = {0};
}

void SIM800C::txCpltCallback() {
    txComplete = true;
}

void SIM800C::rxCpltCallback() {
    rxComplete = true;
}

SIM800CCmdResult* SIM800C::sendCmd(char* cmd, char* expectedResponse, uint16_t receiveTimeoutInTicks, uint8_t numberOfRetries) {
    sim800cCmdResult->cmd = cmd;
    memset(sim800cCmdResult->rxBuffer, 0, sizeof(sim800cCmdResult->rxBuffer));
    HAL_UART_Transmit_IT(huart, (uint8_t*)cmd, strlen(cmd));
    rxComplete = false;
    HAL_UART_Receive_IT(huart, (uint8_t*)sim800cCmdResult->rxBuffer, sizeof(sim800cCmdResult->rxBuffer));
    uint32_t startReceiveTick = HAL_GetTick();
    while (1) {
        if (txComplete) {
            printf("SIM800C TX: %s\r\n---------------\r\n", cmd);
        }

        if (rxComplete) {
            printf("SIM800C RX: %s\r\n---------------\r\n", (char *)sim800cCmdResult->rxBuffer);
            return handleResponse(cmd, expectedResponse, receiveTimeoutInTicks, numberOfRetries);
        }
        
        uint32_t currentTick = HAL_GetTick();
        if (receiveTimeoutInTicks + startReceiveTick < currentTick) {
            printf("SIM800C RX TIMEOUT: %s\r\n---------------\r\n", (char *)sim800cCmdResult->rxBuffer);
            return handleResponse(cmd, expectedResponse, receiveTimeoutInTicks, numberOfRetries);
        }
    }
    return sim800cCmdResult;
}

SIM800CCmdResult* SIM800C::handleResponse(char* cmd, char* expectedResponse, uint16_t receiveTimeoutInTicks, uint8_t numberOfRetries) {
    char *foundExpectedResponse = strstr((char *)sim800cCmdResult->rxBuffer, expectedResponse);
    if(foundExpectedResponse) {
        sim800cCmdResult->status = SIM800C_SUCCESS;
        sim800cCmdResult->foundExpectedResponse = foundExpectedResponse;
        return sim800cCmdResult;
    } else {
        if (numberOfRetries == 0) {
            sim800cCmdResult->status = SIM800C_UNEXPECTED_RESPONSE;
            sim800cCmdResult->foundExpectedResponse = NULL;
            memset(sim800cCmdResult->rxBuffer, 0, sizeof(sim800cCmdResult->rxBuffer));
            return sim800cCmdResult;
        } else {
            HAL_Delay(100);
            return sendCmd(cmd, expectedResponse, receiveTimeoutInTicks, --numberOfRetries);
        }
    }
}