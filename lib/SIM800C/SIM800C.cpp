#include "SIM800C.h"
#include "usart.h"
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

}

void SIM800C::rxCpltCallback() {
    //printf((char*)rxBuffer);
    if (rxBufferIndex > (rxBufferSize - 1)) {
        rxBufferIndex = 0;
    }
    sim800cCmdResult.rxBuffer[rxBufferIndex++] = rxBuffer[0];
    HAL_UART_Receive_IT(huart, rxBuffer, 1);
    HAL_UART_Transmit(&huart2, rxBuffer, 1, 1000);
}

SIM800CCmdResult* SIM800C::sendCmd(char *cmd, char *expectedResponse, uint16_t receiveTimeoutInTicks, uint8_t numberOfRetries) {
    sim800cCmdResult.cmd = cmd;
    rxBufferIndex = 0;
    memset(sim800cCmdResult.rxBuffer, 0, sizeof(sim800cCmdResult.rxBuffer));
      
    HAL_StatusTypeDef result = HAL_UART_Transmit(huart, (uint8_t*)cmd, strlen(cmd), 1000);
    if (result == HAL_OK) {
        printf("SIM800C TX(%d): %s\r\n---------------\r\n", numberOfRetries, sim800cCmdResult.cmd);
    } else {
        printf("SIM800C TX FAIL(%d): %s\r\n---------------\r\n", numberOfRetries, sim800cCmdResult.cmd);
    }

    HAL_UART_Receive_IT(huart, rxBuffer, 1);
    HAL_Delay(100);
    uint16_t _receiveTimeoutInTicks = receiveTimeoutInTicks;
    while (true) {
        char* foundExpectedResponse = strstr((char *)sim800cCmdResult.rxBuffer, expectedResponse);
        if(foundExpectedResponse) {
            if (_receiveTimeoutInTicks != receiveTimeoutInTicks) {
               printf("\r\n"); 
            }
            printf("SIM800C RX(%d): %s\r\n---------------\r\n", numberOfRetries, (char *)sim800cCmdResult.rxBuffer);
            sim800cCmdResult.status = SIM800C_SUCCESS;
            sim800cCmdResult.foundExpectedResponse = foundExpectedResponse;
            return &sim800cCmdResult;
        } 

        if (_receiveTimeoutInTicks == 0) {
            printf("\r\nSIM800C RX TIMEOUT(%d): %s\r\n---------------\r\n", numberOfRetries, (char *)sim800cCmdResult.rxBuffer);
            if (numberOfRetries == 0) {
                sim800cCmdResult.status = SIM800C_UNEXPECTED_RESPONSE;
                sim800cCmdResult.foundExpectedResponse = NULL;
                memset(sim800cCmdResult.rxBuffer, 0, sizeof(sim800cCmdResult.rxBuffer));
                return &sim800cCmdResult;
            } else {
                HAL_Delay(1000);
                return sendCmd(cmd, expectedResponse, receiveTimeoutInTicks, --numberOfRetries);
            }
        }
        printf(".");
        HAL_Delay(200);
		_receiveTimeoutInTicks -= 100;
    }

    return &sim800cCmdResult;
}

SIM800CCmdResult* SIM800C::handleResponse(char* cmd, char* expectedResponse, uint16_t receiveTimeoutInTicks, uint8_t numberOfRetries) {
    char* foundExpectedResponse = strstr((char *)sim800cCmdResult.rxBuffer, expectedResponse);
    if(foundExpectedResponse) {
        sim800cCmdResult.status = SIM800C_SUCCESS;
        sim800cCmdResult.foundExpectedResponse = foundExpectedResponse;
        return &sim800cCmdResult;
    } else {
        if (numberOfRetries == 0) {
            sim800cCmdResult.status = SIM800C_UNEXPECTED_RESPONSE;
            sim800cCmdResult.foundExpectedResponse = NULL;
            memset(sim800cCmdResult.rxBuffer, 0, sizeof(sim800cCmdResult.rxBuffer));
            return &sim800cCmdResult;
        } else {
            HAL_Delay(1000);
            return sendCmd(cmd, expectedResponse, receiveTimeoutInTicks, --numberOfRetries);
        }
    }
}