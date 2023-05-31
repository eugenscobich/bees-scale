#ifndef __HX711_H__
#define __HX711_H__

#include "stm32f1xx_hal.h"

class HX711 {

private:
    TIM_HandleTypeDef* htim;
    GPIO_TypeDef* HX711_DT_GPIOx;
    uint16_t HX711_DT_GPIO_Pin;

    GPIO_TypeDef* HX711_SCK_GPIOx;
    uint16_t HX711_SCK_GPIO_Pin;

    int32_t offset;
    float coeficient;
    bool lock;  

    void _nonBlockingDelay(uint32_t delayInTicks);
    void _hx711Delay(uint16_t time);
    void _setPinAsInput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
    void _setPinAsOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
public:

    HX711(TIM_HandleTypeDef* _htim, GPIO_TypeDef* _HX711_DT_GPIOx, uint16_t _HX711_DT_GPIO_Pin, 
          GPIO_TypeDef* _HX711_SCK_GPIOx, uint16_t _HX711_SCK_GPIO_Pin);

    int32_t getValue();
    int32_t getAverageValue(uint16_t samples);

    void setCoeficient(float coef);
    float getCoeficient();
    void calibrate(int32_t value_noload, int32_t value_load, float scale);
    void setZero(uint16_t samples);
    void setOffset(int32_t _offset);
    float getWeight(uint16_t samples);
    void powerDown();
    void powerUp();
    void disable();

};

#endif // __HX711_H__