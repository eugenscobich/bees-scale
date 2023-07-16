#include "SIM800C.h"
#include "usart.h"
#include <stdio.h>
#include <cstring>
#include <cstdarg>
#define CLASS_NAME "SIM800C.c"
#include "log.h"

SIM800C::SIM800C(UART_HandleTypeDef* _huart, GPIO_TypeDef* _SIM800C_PWR_GPIOx, uint16_t _SIM800C_PWR_GPIO_Pin, GPIO_TypeDef* _SIM800C_DTR_GPIOx, uint16_t _SIM800C_DTR_GPIO_Pin, void(*_updateFunction)()):
    huart(_huart),
    SIM800C_PWR_GPIOx(_SIM800C_PWR_GPIOx),
    SIM800C_PWR_GPIO_Pin(_SIM800C_PWR_GPIO_Pin),
    SIM800C_DTR_GPIOx(_SIM800C_DTR_GPIOx),
    SIM800C_DTR_GPIO_Pin(_SIM800C_DTR_GPIO_Pin),
    updateFunction(_updateFunction)
{
    sim800cCmdResult = {0};
}

void SIM800C::init() {
    HAL_UART_Receive_IT(huart, rxBuffer, 1);
}

void SIM800C::_nonBlockingDelay(uint32_t delayInTicks) {
    startDelayTick = HAL_GetTick();
    while (true) {
        if (startDelayTick + delayInTicks < HAL_GetTick()) {
            return;
        }
        updateFunction();
    }
}

void SIM800C::txCpltCallback() {
    txComplete = true;
}

void SIM800C::rxCpltCallback() {
    sim800cCmdResult.rxBuffer[rxBufferIndex++] = rxBuffer[0];
    if (rxBufferIndex >= rxBufferSize) {
        rxBufferIndex = 0;
    }
    HAL_UART_Receive_IT(huart, rxBuffer, 1);
}

SIM800CCmdResult* SIM800C::_sendCmd(const char *cmd) {
    sim800cCmdResult.cmd = cmd;
    sim800cCmdResult.status = SIM800C_RUNNING;
    rxBufferIndex = 0;
    memset(sim800cCmdResult.rxBuffer, 0, sizeof(sim800cCmdResult.rxBuffer));
    txComplete = false;
    HAL_StatusTypeDef result = HAL_UART_Transmit_IT(huart, (uint8_t*)cmd, strlen(cmd));
    if (result == HAL_OK) {
        startDelayTick = HAL_GetTick();
        while (!txComplete) {
            if (startDelayTick + 100 < HAL_GetTick()) {
                goto tx_error;
            }
            updateFunction();
        }
        txComplete = false;
        result = HAL_UART_Transmit_IT(huart, (uint8_t*)"\r\n", 2);
        if (result == HAL_OK) {
            startDelayTick = HAL_GetTick();
            while (!txComplete) {
                if (startDelayTick + 100 < HAL_GetTick()) {
                    goto tx_error;
                }
                updateFunction();
            }
            logInfo("TX(%d): %s\r\n", sim800cCmdResult.retryCount, sim800cCmdResult.cmd);
            sim800cCmdResult.status = SIM800C_SUCCESS;
            return &sim800cCmdResult;
        } else {
            goto tx_error;
        }
    } else {
        tx_error:
        logError("TX FAIL(%d): %s\r\n", sim800cCmdResult.retryCount, sim800cCmdResult.cmd);
        sim800cCmdResult.status = SIM800C_ERROR;
        return &sim800cCmdResult;
    }

    return &sim800cCmdResult;
}

SIM800CCmdResult* SIM800C::sendCmd(const char *cmd) {
    sim800cCmdResult.retryCount = 0;
    return _sendCmd(cmd);
}


SIM800CCmdResult* SIM800C::sendCmd(const char *cmd, const char *expectedResponse, uint16_t receiveTimeout, uint8_t numberOfRetries) {
    sim800cCmdResult.retryCount = 0;
    _sendCmd(cmd);
    sim800cCmdResult.status = SIM800C_RUNNING;
    return _waitForMessage(expectedResponse, receiveTimeout, numberOfRetries);
}


SIM800CCmdResult* SIM800C::_sendCmd(const char *cmd, const char *expectedResponse, uint16_t receiveTimeout, uint8_t numberOfRetries) {
    _sendCmd(cmd);
    sim800cCmdResult.status = SIM800C_RUNNING;
    return _waitForMessage(expectedResponse, receiveTimeout, numberOfRetries);
}

SIM800CCmdResult* SIM800C::waitForMessage(const char *message, uint16_t receiveTimeout) {
    rxBufferIndex = 0;
    memset(sim800cCmdResult.rxBuffer, 0, sizeof(sim800cCmdResult.rxBuffer));
    return _waitForMessage(message, receiveTimeout, 0);
}

SIM800CCmdResult* SIM800C::_waitForMessage(const char *message, uint16_t receiveTimeout, uint8_t numberOfRetries) {
    uint16_t _receiveTimeout = receiveTimeout;
    startDelayTick = HAL_GetTick();
    while (true) {
        uint32_t currentTick = HAL_GetTick();
        if (startDelayTick + 100 < currentTick) {
            char* foundMessage = strstr((char*)sim800cCmdResult.rxBuffer, message);
            if(foundMessage != NULL) {
                sim800cCmdResult.status = SIM800C_SUCCESS;
                char* foundEndLine = strstr(foundMessage + strlen(message), "\r\n");
                if (foundEndLine != NULL) {
                    logInfo("RX(%d): %s", sim800cCmdResult.retryCount, (char *)sim800cCmdResult.rxBuffer);
                    return &sim800cCmdResult;
                }
            } 
            if (_receiveTimeout == 0) {
                if (numberOfRetries == sim800cCmdResult.retryCount) {
                    if (sim800cCmdResult.status == SIM800C_SUCCESS) {
                        logWarn("RX NO END(%d): %s", sim800cCmdResult.retryCount, (char*)sim800cCmdResult.rxBuffer);
                    } else {
                        sim800cCmdResult.status = SIM800C_TIMEOUT;
                        logError("RX TIMEOUT(%d): %s\r\n", sim800cCmdResult.retryCount, (char*)sim800cCmdResult.rxBuffer);
                    }
                    return &sim800cCmdResult;
                } else {
                    sim800cCmdResult.retryCount++;
                    _nonBlockingDelay(1000);
                    return _sendCmd(sim800cCmdResult.cmd, message, receiveTimeout, numberOfRetries);
                }
            }
            startDelayTick = currentTick;
            _receiveTimeout -= 100;
        }
        updateFunction();
    }

    return &sim800cCmdResult;
}


SIM800CFindInRxBufferResult* SIM800C::findInRxBuffer(uint8_t numberOfArguments, const char* from, ...) {
    va_list vl;
    va_start(vl, from);

    const char* fromMessage = from;
    const char* previousFoundFromMessage = strstr((const char *)sim800cCmdResult.rxBuffer, fromMessage);
    const char* toMessage = NULL;
    if (previousFoundFromMessage) {
        for (uint8_t i = 0; i < numberOfArguments; i++) {            
            toMessage = va_arg(vl, const char*);
            uint8_t startIndex = (uint8_t*)previousFoundFromMessage - sim800cCmdResult.rxBuffer + strlen(fromMessage);
            const char* foundToMessage = strstr(previousFoundFromMessage + strlen(fromMessage), toMessage);
            if (foundToMessage) {
                uint8_t endIndex = (uint8_t*)foundToMessage - sim800cCmdResult.rxBuffer;
                sim800cFindInRxBufferResult.results[i].found = true;
                sim800cFindInRxBufferResult.results[i].value = previousFoundFromMessage + strlen(fromMessage);
                sim800cFindInRxBufferResult.results[i].length = endIndex - startIndex;   
                sim800cFindInRxBufferResult.results[i].valueInt = charArray2int(sim800cFindInRxBufferResult.results[i].value, sim800cFindInRxBufferResult.results[i].length);
                previousFoundFromMessage = foundToMessage;
                fromMessage = toMessage;
            } else {
                sim800cFindInRxBufferResult.results[i].found = false;
                sim800cFindInRxBufferResult.results[i].value = NULL;
                sim800cFindInRxBufferResult.results[i].length = 0;  
                break;
            }         
        }
    }
    va_end(vl);
    return &sim800cFindInRxBufferResult;
}


uint32_t SIM800C::charArray2int (const char* array, uint8_t length) {    
    uint32_t number = 0;
    uint32_t mult = 1;
    for (uint8_t i = length; i > 0; i--) {
        if ((array[i - 1] < '0' || array[i - 1] > '9') && array[i - 1] != '-') {
            return 0;
        }

        if (array[i - 1] == '-') {
            if (number) {
                number = -number;
                break;
            }
        } else {
            number += (array[i - 1] - '0') * mult;
            mult *= 10;
        }
    }
    return number;
}