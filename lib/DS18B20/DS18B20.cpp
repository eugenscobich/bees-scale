#include "DS18B20.h"
#include <stdio.h>

#define INFO  "INFO "
#define DEBUG "DEBUG"
#define WARN  "WARN "
#define ERROR "ERROR"
#define CLASS_NAME "DS18B20.c"

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
        _ds18B20Delay(55);
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
    _ds18B20Delay(1);
    for (int i = 0; i < 8; i++)
    {
        _low();
        _ds18B20Delay(1);
        _high();
        _setPinAsInput();
        _ds18B20Delay(4);
        value >>= 1;
        if (HAL_GPIO_ReadPin(DS18B20_DT_GPIOx, DS18B20_DT_GPIO_Pin)) // if the pin is HIGH
        {
            value |= 0x80;
        }
        _ds18B20Delay(40);
        _setPinAsOutput();
        _high();
    }
    return value;
}

// #############################################################################################
bool DS18B20::readRom()
{
    write(0x33); // read ROM
    for (uint8_t j = 0; j < 8; j++)
    {
        rom[j] = read();
    }
    printf("%010lu [%s] %s: Read Rom=%02X%02X%02X%02X%02X%02X%02X%02X\r\n", HAL_GetTick(), DEBUG, CLASS_NAME, rom[0], rom[1], rom[2], rom[3], rom[4], rom[5], rom[6], rom[7]);
    return verifyRomCrc();
}

// #############################################################################################
uint8_t* DS18B20::getRom()
{
    return rom;
}

// #############################################################################################
void DS18B20::waitForTemperatureRead()
{
    _high();
    _ds18B20Delay(1);
    _setPinAsOutput();
    _low();
    _ds18B20Delay(1);
    _setPinAsInput();
    _ds18B20Delay(15);
    uint32_t tick = HAL_GetTick();
    while (HAL_GPIO_ReadPin(DS18B20_DT_GPIOx, DS18B20_DT_GPIO_Pin) == GPIO_PIN_RESET) {
        if (tick + 800 < HAL_GetTick())
        {
            return;
        }
    }
}

bool DS18B20::saveBytes(uint8_t th, uint8_t tl) {
    start();
    readScratchpad();
    start();
    write(0xCC);
    write(0x4E);
    write(th);
    write(tl);
    write(scratchpad[4]);
    start();
    if (readScratchpad() && scratchpad[2] == th && scratchpad[3] == tl) {
        start();
        write(0xCC);
        write(0x48);
        HAL_Delay(10);
        return true;
    }
    return false;
}

// #############################################################################################
bool DS18B20::readScratchpad() {
    write(0xCC);
    write(0xBE);
    for (uint8_t i = 0; i < 9; i++)
    {
        scratchpad[i] = read();        
    }
    printf("%010lu [%s] %s: Read scratchpad=%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", HAL_GetTick(), DEBUG, CLASS_NAME, 
        scratchpad[0], scratchpad[1], scratchpad[2], scratchpad[3], scratchpad[4], scratchpad[5], scratchpad[6], scratchpad[7], scratchpad[8]);
    return verifyScratchpadCrc();
}

// #############################################################################################
uint8_t* DS18B20::getScratchpad() {
    return scratchpad;
}

// #############################################################################################
bool DS18B20::readTemperature() {
    temperature = 0;
    write(0x44);
    //waitForTemperatureRead();
    HAL_Delay(800);
    start();
    if (readScratchpad()) {
        uint8_t lsByte = scratchpad[0];
        temperature = ((scratchpad[1] << 8) | lsByte) * 0.0625;
        temperature = temperature - 0.25 + ((scratchpad[7] - scratchpad[6]) / scratchpad[7]);
        return true;
    }
    return false;
}

// #############################################################################################
float DS18B20::getTemperature() {
    return temperature;
}

// #############################################################################################
bool DS18B20::verifyScratchpadCrc() {
    return _verifyCrc(scratchpad, sizeof(scratchpad));
}

// #############################################################################################
bool DS18B20::verifyRomCrc() {
    return _verifyCrc(rom, sizeof(rom));
}

// #############################################################################################
bool DS18B20::_verifyCrc(uint8_t* bytes, uint8_t arrayLength) {
    uint8_t lastByte = bytes[arrayLength - 1];
    uint8_t	crc = CRC8INIT;
    uint8_t feedback_bit;
    for (uint8_t i = 0; i < arrayLength - 1; i++)
	{
		uint8_t data = bytes[i];
        for (uint8_t j = 8; j; j--) {
			feedback_bit = (crc ^ data) & 0x01;
			crc >>= 1;
			if (feedback_bit) {
				crc ^= 0x8C;
			}
			data >>= 1;
		}
	}
    return lastByte == crc;
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