#ifndef __SIM800C_H__
#define __SIM800C_H__

#include "stm32f1xx_hal.h"
#include <stdio.h>


#define SIM800C_STATE_UNKNOWN           0
#define SIM800C_STATE_RUNNING           1
#define SIM800C_STATE_TX_COMPLETED      2
#define SIM800C_STATE_RX_COMPLETED      3
#define SIM800C_STATE_WAITING_RESPONSE  4
#define SIM800C_STATE_PATTERN_MATCHED   5
#define SIM800C_STATE_ERROR             6

typedef struct {
    char* cmd;
    uint8_t rxBuffer[100];
    uint8_t previousState;
    uint8_t currentState;
} SIM800CState;

class SIM800C {

private:
    UART_HandleTypeDef* huart;

    GPIO_TypeDef* SIM800C_PWR_GPIOx;
    uint16_t SIM800C_PWR_GPIO_Pin;

    GPIO_TypeDef* SIM800C_DTR_GPIOx;
    uint16_t SIM800C_DTR_GPIO_Pin;

    SIM800CState sim800cState;

    char* expectedPattern;

    volatile bool txComplete = false;
    volatile bool rxComplete = false;

    uint32_t startReceiveTick;
    uint16_t receiveTimeoutInTicks;

    void setState(uint8_t newState);
    
public:
    SIM800C(UART_HandleTypeDef* _huart, GPIO_TypeDef* _SIM800C_PWR_GPIOx, uint16_t _SIM800C_PWR_GPIO_Pin, GPIO_TypeDef* _SIM800C_DTR_GPIOx, uint16_t _SIM800C_DTR_GPIO_Pin);
    SIM800CState update();
    void rxCpltCallback();
    void txCpltCallback();
    void sendCmdAndSetExpectedPatternAndTimeout(char* command, char* expectedPattern, uint16_t timeout);
};

#endif // __SIM800C_H__