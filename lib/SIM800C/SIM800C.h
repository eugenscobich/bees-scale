#ifndef __SIM800C_H__
#define __SIM800C_H__

#include "stm32f1xx_hal.h"
#include <stdio.h>

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
    uint8_t rxBuffer[100];
} SIM800CCmdResult;

class SIM800C {

private:
    UART_HandleTypeDef* huart;

    GPIO_TypeDef* SIM800C_PWR_GPIOx;
    uint16_t SIM800C_PWR_GPIO_Pin;

    GPIO_TypeDef* SIM800C_DTR_GPIOx;
    uint16_t SIM800C_DTR_GPIO_Pin;

    SIM800CCmdResult* sim800cCmdResult;

    char* expectedResponse;

    volatile bool txComplete = false;
    volatile bool rxComplete = false;

    SIM800CCmdResult* handleResponse(char* cmd, char* expectedResponse, uint16_t receiveTimeoutInTicks, uint8_t numberOfRetries = 0);
    
public:
    SIM800C(UART_HandleTypeDef* _huart, GPIO_TypeDef* _SIM800C_PWR_GPIOx, uint16_t _SIM800C_PWR_GPIO_Pin, GPIO_TypeDef* _SIM800C_DTR_GPIOx, uint16_t _SIM800C_DTR_GPIO_Pin);
    void rxCpltCallback();
    void txCpltCallback();
    SIM800CCmdResult* sendCmd(char* cmd, char* expectedResponse, uint16_t receiveTimeoutInTicks, uint8_t numberOfRetries = 0);
};

#endif // __SIM800C_H__