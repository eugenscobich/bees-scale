#include "DS18B20.h"

DS18B20::DS18B20(TIM_HandleTypeDef *_htim, GPIO_TypeDef *_DS18B20_DT_GPIOx, uint16_t _DS18B20_DT_GPIO_Pin) : htim(_htim),
                                                                                                             DS18B20_DT_GPIOx(_DS18B20_DT_GPIOx),
                                                                                                             DS18B20_DT_GPIO_Pin(_DS18B20_DT_GPIO_Pin)
{
}

void DS18B20::_ds18B20Delay(uint16_t us)
{
    
    __HAL_TIM_SET_COUNTER(htim, 0);
    while ((__HAL_TIM_GET_COUNTER(htim)) < us)
    {
    }


    // __IO uint32_t currentTicks = SysTick->VAL;
    // /* Number of ticks per millisecond */
    // const uint32_t tickPerMs = SysTick->LOAD + 1;
    // /* Number of ticks to count */
    // const uint32_t nbTicks = ((us - ((us > 0) ? 1 : 0)) * tickPerMs) / 1000;
    // /* Number of elapsed ticks */
    // uint32_t elapsedTicks = 0;
    // __IO uint32_t oldTicks = currentTicks;
    // do
    // {
    //     currentTicks = SysTick->VAL;
    //     elapsedTicks += (oldTicks < currentTicks) ? tickPerMs + oldTicks - currentTicks : oldTicks - currentTicks;
    //     oldTicks = currentTicks;
    // } while (nbTicks > elapsedTicks);
    
}    

// #############################################################################################
void DS18B20::_low()
{
    HAL_GPIO_WritePin(DS18B20_DT_GPIOx, DS18B20_DT_GPIO_Pin, GPIO_PIN_RESET);
}

// #############################################################################################
void DS18B20::_high()
{
    HAL_GPIO_WritePin(DS18B20_DT_GPIOx, DS18B20_DT_GPIO_Pin, GPIO_PIN_SET);
}

// #############################################################################################
bool DS18B20::start()
{
    /*
    _setPinAsOutput();
    while (1) {
        HAL_GPIO_TogglePin(DS18B20_DT_GPIOx, DS18B20_DT_GPIO_Pin);
        _ds18B20Delay(12);
    }
*/
    _setPinAsOutput();
    _high();
    _ds18B20Delay(5);
    _low();
    _ds18B20Delay(750); // 480-960
    _high();
    _setPinAsInput();
    _ds18B20Delay(80);
    uint16_t usWaitResponse = 0;
    while (HAL_GPIO_ReadPin(DS18B20_DT_GPIOx, DS18B20_DT_GPIO_Pin) == GPIO_PIN_SET)
    {
        usWaitResponse++;
        if (usWaitResponse > 60)
        {
            return false; // if the pin is not low the presence pulse is not detected
        }
        _ds18B20Delay(1);
    }
    usWaitResponse = 480 - usWaitResponse;
    _setPinAsOutput();
    _ds18B20Delay(usWaitResponse);
    _high();
    return true;
}

// #############################################################################################
void DS18B20::write(uint8_t data)
{
    _setPinAsOutput();
    for (int i = 0; i < 8; i++)
    {
        _low();
        _ds18B20Delay(10);
        if ((data & (1 << i)) != 0) // if the bit is high
        {
            _high();
        }
        _ds18B20Delay(50);
        _high();
    }
}
// #############################################################################################
void DS18B20::startReadTimeSlot()
{
    _setPinAsOutput();
    _low();
    _ds18B20Delay(2);
    _setPinAsInput();
}

// #############################################################################################
uint8_t DS18B20::read()
{
    uint8_t value = 0;
    _setPinAsOutput();
    _high();
    _ds18B20Delay(2);
    for (int i = 0; i < 8; i++)
    {
        _low();
        _ds18B20Delay(1);
        _high();
        _setPinAsInput();
        _ds18B20Delay(5);
        value >>= 1;
        if (HAL_GPIO_ReadPin(DS18B20_DT_GPIOx, DS18B20_DT_GPIO_Pin)) // if the pin is HIGH
        {
            value |= 0x80;
        }
        _ds18B20Delay(55);
        _setPinAsOutput();
        _high();
    }
    return value;
}

// #############################################################################################
void DS18B20::_setPinAsOutput()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_DT_GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_DT_GPIOx, &GPIO_InitStruct);
}

// #############################################################################################
void DS18B20::_setPinAsInput()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_DT_GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_DT_GPIOx, &GPIO_InitStruct);
}