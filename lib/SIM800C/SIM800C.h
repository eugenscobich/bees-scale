#ifndef __SIM800C_H__
#define __SIM800C_H__

#include "stm32f1xx_hal.h"
#include <stdio.h>

#define RX_BUFFER_SIZE 256

typedef enum {
    SIM800C_RUNNING,
	SIM800C_SUCCESS,
	SIM800C_ERROR,
	SIM800C_TIMEOUT,
	SIM800C_NO_RESPONSE,
	SIM800C_UNEXPECTED_RESPONSE,
	SIM800C_OVERFLOW,
	SIM800C_BAD_PARAMETER
} SIM800CCmdResultStatus;

typedef struct {
    char* cmd;
    SIM800CCmdResultStatus status;
    uint8_t retryCount;
    uint8_t rxBuffer[RX_BUFFER_SIZE];
} SIM800CCmdResult;

typedef struct {
    bool found;
    bool secondFound;
    bool thirdFound;
    bool forthFound;
    bool fifthFound;
    bool sixthFound;
    char* value;
    char* secondValue;
    char* thirdValue;
    char* forthValue;
    char* fifthValue;
    char* sixthValue;
    uint32_t valueInt;
    uint32_t secondValueInt;
    uint32_t thirdValueInt;
    uint8_t length;
    uint8_t secondLength;
    uint8_t thirdLength;
    uint8_t forthLength;
    uint8_t fifthLength;
    uint8_t sixthLength;
} SIM800CFindInRxBufferResult;

class SIM800C {

private:
    UART_HandleTypeDef* huart;

    GPIO_TypeDef* SIM800C_PWR_GPIOx;
    uint16_t SIM800C_PWR_GPIO_Pin;

    GPIO_TypeDef* SIM800C_DTR_GPIOx;
    uint16_t SIM800C_DTR_GPIO_Pin;

    SIM800CCmdResult sim800cCmdResult;
    SIM800CFindInRxBufferResult sim800cFindInRxBufferResult;

    char* expectedResponse;

    volatile bool txComplete = false;
    volatile bool rxComplete = false;
    
    uint16_t rxBufferSize = RX_BUFFER_SIZE;
    uint8_t rxBuffer[1] = { 0 };
    uint16_t rxBufferIndex = 0;

    SIM800CCmdResult* _waitForMessage(char *message, uint16_t waitTimeout, uint8_t numberOfRetries);
    SIM800CCmdResult* _sendCmd(char *cmd);
    SIM800CCmdResult* _sendCmd(char *cmd, char *expectedResponse, uint16_t receiveTimeout, uint8_t numberOfRetries);
    uint32_t _charArray2int (char *array, uint8_t n);

public:
    SIM800C(UART_HandleTypeDef* _huart, GPIO_TypeDef* _SIM800C_PWR_GPIOx, uint16_t _SIM800C_PWR_GPIO_Pin, GPIO_TypeDef* _SIM800C_DTR_GPIOx, uint16_t _SIM800C_DTR_GPIO_Pin);
    void init();
    void rxCpltCallback();
    void txCpltCallback();
    SIM800CCmdResult* sendCmd(char *cmd);
    SIM800CCmdResult* sendCmd(char* cmd, char* expectedResponse, uint16_t receiveTimeout = 1000, uint8_t numberOfRetries = 0);
    SIM800CCmdResult* waitForMessage(char *message, uint16_t waitTimeout);

    SIM800CFindInRxBufferResult* findInRxBuffer(char* from, char* to, char* secondTo = NULL, char* thirdTo = NULL, char* forthTo = NULL, char* fifthTo = NULL, char* sixthTo = NULL);
    SIM800CFindInRxBufferResult* findInRxBufferAndParseToInt(char* from, char* to, char* secondTo = NULL, char* thirdTo = NULL);
};

#endif // __SIM800C_H__