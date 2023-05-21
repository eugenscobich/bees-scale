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
    const char* cmd;
    SIM800CCmdResultStatus status;
    uint8_t retryCount;
    uint8_t rxBuffer[RX_BUFFER_SIZE];
} SIM800CCmdResult;

typedef struct {
    bool found;
    const char* value;
    uint32_t valueInt;
    uint8_t length;
} SIM800CFindInRxBufferSingleResult;

typedef struct {
    SIM800CFindInRxBufferSingleResult results[7];
} SIM800CFindInRxBufferResult;

class SIM800C {

private:
    UART_HandleTypeDef* huart;

    GPIO_TypeDef* SIM800C_PWR_GPIOx;
    uint16_t SIM800C_PWR_GPIO_Pin;

    GPIO_TypeDef* SIM800C_DTR_GPIOx;
    uint16_t SIM800C_DTR_GPIO_Pin;

    void(*updateFunction)();

    SIM800CCmdResult sim800cCmdResult;
    SIM800CFindInRxBufferResult sim800cFindInRxBufferResult;

    char* expectedResponse;

    volatile bool txComplete = false;
    volatile bool rxComplete = false;
    
    uint32_t startDelayTick;
    
    uint16_t rxBufferSize = RX_BUFFER_SIZE;
    uint8_t rxBuffer[1] = { 0 };
    uint16_t rxBufferIndex = 0;

    SIM800CCmdResult* _waitForMessage(const char *message, uint16_t waitTimeout, uint8_t numberOfRetries);
    SIM800CCmdResult* _sendCmd(const char *cmd);
    SIM800CCmdResult* _sendCmd(const char *cmd, const char *expectedResponse, uint16_t receiveTimeout, uint8_t numberOfRetries);
    uint32_t _charArray2int (const char *array, uint8_t n);

public:

    SIM800C(UART_HandleTypeDef* _huart, GPIO_TypeDef* _SIM800C_PWR_GPIOx, uint16_t _SIM800C_PWR_GPIO_Pin, GPIO_TypeDef* _SIM800C_DTR_GPIOx, uint16_t _SIM800C_DTR_GPIO_Pin, void(*updateFunction)());
    
    void _nonBlockingDelay(uint32_t delayInTicks);
    void init();
    void rxCpltCallback();
    void txCpltCallback();
    SIM800CCmdResult* sendCmd(const char *cmd);
    SIM800CCmdResult* sendCmd(const char* cmd, const char* expectedResponse, uint16_t receiveTimeout = 1000, uint8_t numberOfRetries = 0);
    SIM800CCmdResult* waitForMessage(const char *message, uint16_t waitTimeout);

    SIM800CFindInRxBufferResult* findInRxBuffer(uint8_t numberOfArguments, const char* from, ...);
    SIM800CFindInRxBufferResult* findInRxBufferAndParseToInt(const char* from ...);
};

#endif // __SIM800C_H__