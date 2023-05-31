#ifndef __DS18B20_H__
#define __DS18B20_H__

#include "stm32f1xx_hal.h"

class DS18B20 {

private:
    TIM_HandleTypeDef* htim;
    GPIO_TypeDef* DS18B20_DT_GPIOx;
    uint16_t DS18B20_DT_GPIO_Pin;
    
    uint32_t startDelayTick;

    int32_t offset;
    float coeficient;
    bool lock;  

    void _ds18B20Delay(uint16_t time);

    void _low();
    void _high();
    void _setPinAsInput();
    void _setPinAsOutput();

public:

    DS18B20(TIM_HandleTypeDef* _htim, GPIO_TypeDef* _DS18B20_DT_GPIOx, uint16_t _DS18B20_DT_GPIO_Pin);

    bool start();
    void write(uint8_t data);
    uint8_t read();
    void startReadTimeSlot();

};




#endif // __DS18B20_H__