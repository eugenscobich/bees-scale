#include "HX711.h"

HX711::HX711(TIM_HandleTypeDef *_htim, GPIO_TypeDef *_HX711_DT_GPIOx, uint16_t _HX711_DT_GPIO_Pin,
             GPIO_TypeDef *_HX711_SCK_GPIOx, uint16_t _HX711_SCK_GPIO_Pin) : htim(_htim), HX711_DT_GPIOx(_HX711_DT_GPIOx),
                                                                             HX711_DT_GPIO_Pin(_HX711_DT_GPIO_Pin),
                                                                             HX711_SCK_GPIOx(_HX711_SCK_GPIOx),
                                                                             HX711_SCK_GPIO_Pin(_HX711_SCK_GPIO_Pin)
{
}

void HX711::_hx711Delay(uint16_t time)
{
    __HAL_TIM_SET_COUNTER(htim, 0);
    while ((__HAL_TIM_GET_COUNTER(htim)) < time)
    {
    }
}

// #############################################################################################
int32_t HX711::getValue()
{
    uint32_t data = 0;
    uint32_t startTime = HAL_GetTick();
    _setPinAsInput(HX711_DT_GPIOx, HX711_DT_GPIO_Pin);
    HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_RESET);
    while (HAL_GPIO_ReadPin(HX711_DT_GPIOx, HX711_DT_GPIO_Pin) == GPIO_PIN_SET)
    {
        if (HAL_GetTick() - startTime > 400)
        {
            return 0;
        }
    }

    for (int8_t i = 0; i < 24; i++)
    {
        HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_SET);
        data = data << 1;
        _hx711Delay(1);
        HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_RESET);
        if (HAL_GPIO_ReadPin(HX711_DT_GPIOx, HX711_DT_GPIO_Pin))
        {
            data++;
        }
    }

    HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_SET);
    _hx711Delay(1);
    HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_RESET);

    data = data ^ 0x800000;

    return data;
}
// #############################################################################################
int32_t HX711::getAverageValue(uint16_t samples)
{
    int64_t average = 0;
    for (uint16_t i = 0; i < samples; i++)
    {
        average += getValue();
    }
    return (int32_t)(average / samples);
}
// #############################################################################################
void HX711::setZero(uint16_t samples)
{
    offset = getAverageValue(samples);
}

// #############################################################################################
void HX711::setOffset(int32_t _offset)
{
    offset = _offset;
}
// #############################################################################################
void HX711::calibrate(int32_t noload_raw, int32_t load_raw, float scale)
{
    offset = noload_raw;
    coeficient = (load_raw - noload_raw) / scale;
}
// #############################################################################################
float HX711::getWeight(uint16_t samples)
{
    return (getAverageValue(samples) - offset) / coeficient;
}
// #############################################################################################
void HX711::setCoeficient(float _coeficient)
{
    coeficient = _coeficient;
}
// #############################################################################################
float HX711::getCoeficient()
{
    return coeficient;
}
// #############################################################################################
void HX711::powerDown()
{
    HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_SET);
    _nonBlockingDelay(1);
}
// #############################################################################################
void HX711::powerUp()
{
    HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_RESET);
}
// #############################################################################################
void HX711::disable()
{
    HAL_GPIO_WritePin(HX711_SCK_GPIOx, HX711_SCK_GPIO_Pin, GPIO_PIN_SET);
    _hx711Delay(100);
}
// #############################################################################################

void HX711::_setPinAsInput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}