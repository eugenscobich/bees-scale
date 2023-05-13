#ifndef __SIM800C_H__
#define __SIM800C_H__

#include "stm32f1xx_hal.h"
#include <stdio.h>

#define RX_BUFFER_SIZE 256

typedef enum {
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
    char* foundExpectedResponse;
    uint8_t rxBuffer[RX_BUFFER_SIZE];
} SIM800CCmdResult;

class SIM800C {

private:
    UART_HandleTypeDef* huart;

    GPIO_TypeDef* SIM800C_PWR_GPIOx;
    uint16_t SIM800C_PWR_GPIO_Pin;

    GPIO_TypeDef* SIM800C_DTR_GPIOx;
    uint16_t SIM800C_DTR_GPIO_Pin;

    SIM800CCmdResult sim800cCmdResult;

    char* expectedResponse;

    volatile bool txComplete = false;
    volatile bool rxComplete = false;
    
    uint16_t rxBufferSize = RX_BUFFER_SIZE;
    uint8_t rxBuffer[1] = { 0 };
    uint16_t rxBufferIndex = 0;

    SIM800CCmdResult* handleResponse(char* cmd, char* expectedResponse, uint16_t receiveTimeoutInTicks, uint8_t numberOfRetries);
    
public:
    SIM800C(UART_HandleTypeDef* _huart, GPIO_TypeDef* _SIM800C_PWR_GPIOx, uint16_t _SIM800C_PWR_GPIO_Pin, GPIO_TypeDef* _SIM800C_DTR_GPIOx, uint16_t _SIM800C_DTR_GPIO_Pin);
    void rxCpltCallback();
    void txCpltCallback();
    SIM800CCmdResult* sendCmd(char* cmd, char* expectedResponse, uint16_t receiveTimeoutInTicks = 1000, uint8_t numberOfRetries = 0);
};

#endif // __SIM800C_H__