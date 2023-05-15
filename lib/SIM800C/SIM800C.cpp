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

void SIM800C::init() {
    HAL_UART_Receive_IT(huart, rxBuffer, 1);
}

void SIM800C::txCpltCallback() {

}

void SIM800C::rxCpltCallback() {
    sim800cCmdResult.rxBuffer[rxBufferIndex++] = rxBuffer[0];
    if (rxBufferIndex >= rxBufferSize) {
        rxBufferIndex = 0;
    }
    
    //printf((char*)rxBuffer);
    //HAL_UART_Transmit(&huart2, rxBuffer, 1, 100);
    HAL_UART_Receive_IT(huart, rxBuffer, 1);
}

SIM800CCmdResult* SIM800C::_sendCmd(char *cmd) {
    sim800cCmdResult.cmd = cmd;
    sim800cCmdResult.status = SIM800C_RUNNING;
    rxBufferIndex = 0;
    memset(sim800cCmdResult.rxBuffer, 0, sizeof(sim800cCmdResult.rxBuffer));

    HAL_StatusTypeDef result = HAL_UART_Transmit(huart, (uint8_t*)cmd, strlen(cmd), 100);
    result = HAL_UART_Transmit(huart, (uint8_t*)"\r\n", 2, 100);
    if (result == HAL_OK) {
        printf("SIM800C TX(%d): %s\r\n", sim800cCmdResult.retryCount, sim800cCmdResult.cmd);
        sim800cCmdResult.status = SIM800C_SUCCESS;
        return &sim800cCmdResult;
    } else {
        printf("SIM800C TX FAIL(%d): %s\r\n", sim800cCmdResult.retryCount, sim800cCmdResult.cmd);
        sim800cCmdResult.status = SIM800C_ERROR;
        return &sim800cCmdResult;
    }

    return &sim800cCmdResult;
}

SIM800CCmdResult* SIM800C::sendCmd(char *cmd) {
    sim800cCmdResult.retryCount = 0;
    return _sendCmd(cmd);
}


SIM800CCmdResult* SIM800C::sendCmd(char *cmd, char *expectedResponse, uint16_t receiveTimeout, uint8_t numberOfRetries) {
    sim800cCmdResult.retryCount = 0;
    _sendCmd(cmd);
    sim800cCmdResult.status = SIM800C_RUNNING;
    return _waitForMessage(expectedResponse, receiveTimeout, numberOfRetries);
}


SIM800CCmdResult* SIM800C::_sendCmd(char *cmd, char *expectedResponse, uint16_t receiveTimeout, uint8_t numberOfRetries) {
    _sendCmd(cmd);
    sim800cCmdResult.status = SIM800C_RUNNING;
    return _waitForMessage(expectedResponse, receiveTimeout, numberOfRetries);
}

SIM800CCmdResult* SIM800C::waitForMessage(char *message, uint16_t receiveTimeout) {
    rxBufferIndex = 0;
    memset(sim800cCmdResult.rxBuffer, 0, sizeof(sim800cCmdResult.rxBuffer));
    return _waitForMessage(message, receiveTimeout, 0);
}

SIM800CCmdResult* SIM800C::_waitForMessage(char *message, uint16_t receiveTimeout, uint8_t numberOfRetries) {
    uint16_t _receiveTimeout = receiveTimeout;
    while (true) {
        char* foundMessage = strstr((char*)sim800cCmdResult.rxBuffer, message);
        if(foundMessage != NULL) {
            sim800cCmdResult.status = SIM800C_SUCCESS;
            char* foundEndLine = strstr(foundMessage + strlen(message), "\r\n");
            if (foundEndLine != NULL) {
                printf("SIM800C RX(%d): %s", sim800cCmdResult.retryCount, (char *)sim800cCmdResult.rxBuffer);
                return &sim800cCmdResult;
            }
        } 

        if (_receiveTimeout == 0) {
            if (numberOfRetries == sim800cCmdResult.retryCount) {
                if (sim800cCmdResult.status == SIM800C_SUCCESS) {
                    printf("SIM800C RX NO END(%d): %s", sim800cCmdResult.retryCount, (char*)sim800cCmdResult.rxBuffer);
                } else {
                    sim800cCmdResult.status = SIM800C_TIMEOUT;
                    printf("SIM800C RX TIMEOUT(%d): %s\r\n", sim800cCmdResult.retryCount, (char*)sim800cCmdResult.rxBuffer);
                }
                return &sim800cCmdResult;
            } else {
                sim800cCmdResult.retryCount++;
                HAL_Delay(1000);
                return _sendCmd(sim800cCmdResult.cmd, message, receiveTimeout, numberOfRetries);
            }
        }
        HAL_Delay(100);
		_receiveTimeout -= 100;
    }

    return &sim800cCmdResult;
}


SIM800CFindInRxBufferResult* SIM800C::findInRxBuffer(char* from, char* to, char* secondTo, char* thirdTo, char* forthTo, char* fifthTo, char* sixthTo) {
    sim800cFindInRxBufferResult.found = false;
    sim800cFindInRxBufferResult.value = NULL;
    sim800cFindInRxBufferResult.length = 0;
    sim800cFindInRxBufferResult.secondFound = false;
    sim800cFindInRxBufferResult.secondValue = NULL;
    sim800cFindInRxBufferResult.secondLength = 0;
    sim800cFindInRxBufferResult.thirdFound = false;
    sim800cFindInRxBufferResult.thirdValue = NULL;
    sim800cFindInRxBufferResult.thirdLength = 0;
    sim800cFindInRxBufferResult.forthValue = NULL;
    sim800cFindInRxBufferResult.forthLength = 0;
    sim800cFindInRxBufferResult.fifthValue = NULL;
    sim800cFindInRxBufferResult.fifthLength = 0;
    sim800cFindInRxBufferResult.sixthValue = NULL;
    sim800cFindInRxBufferResult.sixthLength = 0;

    char* foundFromMessage = strstr((char *)sim800cCmdResult.rxBuffer, from);
    if (foundFromMessage) {
        uint8_t startIndex = (uint8_t*)foundFromMessage - sim800cCmdResult.rxBuffer + strlen(from);
        char* foundToMessage = strstr(foundFromMessage + strlen(from), to);
        if (foundToMessage) {
            uint8_t endIndex = (uint8_t*)foundToMessage - sim800cCmdResult.rxBuffer;
            sim800cFindInRxBufferResult.found = true;
            sim800cFindInRxBufferResult.value = foundFromMessage + strlen(from);
            sim800cFindInRxBufferResult.length = endIndex - startIndex;

            if (secondTo != NULL) {
                startIndex = (uint8_t*)foundToMessage - sim800cCmdResult.rxBuffer + strlen(to);
                char* foundSecondToMessage = strstr(foundToMessage + strlen(to), secondTo);
                if (foundSecondToMessage) {
                    endIndex = (uint8_t*)foundSecondToMessage - sim800cCmdResult.rxBuffer;
                    sim800cFindInRxBufferResult.secondFound = true;
                    sim800cFindInRxBufferResult.secondValue = foundToMessage + strlen(to);
                    sim800cFindInRxBufferResult.secondLength = endIndex - startIndex;

                    if (thirdTo != NULL) {
                        startIndex = (uint8_t*)foundSecondToMessage - sim800cCmdResult.rxBuffer + strlen(secondTo);
                        char* foundThirdToMessage = strstr(foundSecondToMessage + strlen(secondTo), thirdTo);
                        if (foundThirdToMessage) {
                            endIndex = (uint8_t*)foundThirdToMessage - sim800cCmdResult.rxBuffer;
                            sim800cFindInRxBufferResult.thirdFound = true;
                            sim800cFindInRxBufferResult.thirdValue = foundSecondToMessage + strlen(secondTo);
                            sim800cFindInRxBufferResult.thirdLength = endIndex - startIndex;
                        }

                        if (forthTo != NULL) {
                            startIndex = (uint8_t*)foundThirdToMessage - sim800cCmdResult.rxBuffer + strlen(thirdTo);
                            char* foundForthToMessage = strstr(foundThirdToMessage + strlen(thirdTo), forthTo);
                            if (foundForthToMessage) {
                                endIndex = (uint8_t*)foundForthToMessage - sim800cCmdResult.rxBuffer;
                                sim800cFindInRxBufferResult.forthFound = true;
                                sim800cFindInRxBufferResult.forthValue = foundThirdToMessage + strlen(thirdTo);
                                sim800cFindInRxBufferResult.forthLength = endIndex - startIndex;
                            } 

                            if (fifthTo != NULL) {
                                startIndex = (uint8_t*)foundForthToMessage - sim800cCmdResult.rxBuffer + strlen(forthTo);
                                char* foundFifthToMessage = strstr(foundForthToMessage + strlen(forthTo), fifthTo);
                                if (foundFifthToMessage) {
                                    endIndex = (uint8_t*)foundFifthToMessage - sim800cCmdResult.rxBuffer;
                                    sim800cFindInRxBufferResult.fifthFound = true;
                                    sim800cFindInRxBufferResult.fifthValue = foundForthToMessage + strlen(forthTo);
                                    sim800cFindInRxBufferResult.fifthLength = endIndex - startIndex;
                                } 

                                if (sixthTo != NULL) {
                                    startIndex = (uint8_t*)foundFifthToMessage - sim800cCmdResult.rxBuffer + strlen(fifthTo);
                                    char* foundSixthToMessage = strstr(foundFifthToMessage + strlen(fifthTo), sixthTo);
                                    if (foundSixthToMessage) {
                                        endIndex = (uint8_t*)foundSixthToMessage - sim800cCmdResult.rxBuffer;
                                        sim800cFindInRxBufferResult.sixthFound = true;
                                        sim800cFindInRxBufferResult.sixthValue = foundFifthToMessage + strlen(fifthTo);
                                        sim800cFindInRxBufferResult.sixthLength = endIndex - startIndex;
                                    } 
                                }
                            }
                        }
                    }
                } 
            } 
        } 
    }

    return &sim800cFindInRxBufferResult;
}

SIM800CFindInRxBufferResult* SIM800C::findInRxBufferAndParseToInt(char* from, char* to, char* secondTo, char* thirdTo) {
    findInRxBuffer(from, to, secondTo, thirdTo);
    sim800cFindInRxBufferResult.valueInt = 0;
    sim800cFindInRxBufferResult.secondValueInt = 0;
    sim800cFindInRxBufferResult.thirdValueInt = 0;
    if (sim800cFindInRxBufferResult.found) {
        sim800cFindInRxBufferResult.valueInt = _charArray2int(sim800cFindInRxBufferResult.value, sim800cFindInRxBufferResult.length);
    }
    if (sim800cFindInRxBufferResult.secondFound) {
        sim800cFindInRxBufferResult.secondValueInt = _charArray2int(sim800cFindInRxBufferResult.secondValue, sim800cFindInRxBufferResult.secondLength);
    }
    if (sim800cFindInRxBufferResult.thirdFound) {
        sim800cFindInRxBufferResult.thirdValueInt = _charArray2int(sim800cFindInRxBufferResult.thirdValue, sim800cFindInRxBufferResult.thirdLength);
    }
    return &sim800cFindInRxBufferResult;
}

uint32_t SIM800C::_charArray2int (char* array, uint8_t length) {    
    uint32_t number = 0;
    uint32_t mult = 1;
    for (uint8_t i = length; i > 0; i--) {
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