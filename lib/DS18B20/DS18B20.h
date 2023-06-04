#ifndef __DS18B20_H__
#define __DS18B20_H__

#include "stm32f1xx_hal.h"

#define CRC8INIT	0x00
#define CRC8POLY	0x18              //0X18 = X^8+X^5+X^4+X^0

class DS18B20 {

private:
    TIM_HandleTypeDef* htim;
    GPIO_TypeDef* DS18B20_DT_GPIOx;
    uint16_t DS18B20_DT_GPIO_Pin;
    
    uint8_t rom[8];
    uint8_t scratchpad[9];
    float temperature;

    void _ds18B20Delay(uint16_t time);

    void _low();
    void _high();
    void _setPinAsInput();
    void _setPinAsOutput();
    bool _verifyCrc(uint8_t* bytes, uint8_t arrayLength);

public:

    DS18B20(TIM_HandleTypeDef* _htim, GPIO_TypeDef* _DS18B20_DT_GPIOx, uint16_t _DS18B20_DT_GPIO_Pin);

    void waitForTemperatureRead();
    bool start();
    void write(uint8_t data);
    uint8_t read();
    bool readScratchpad();
    bool readRom();
    uint8_t* getScratchpad();
    uint8_t* getRom();
    bool verifyScratchpadCrc();
    bool verifyRomCrc();
    void startReadTimeSlot();
    bool saveBytes(uint8_t lsb, uint8_t msb);
    bool readTemperature();
    float getTemperature();
};




#endif // __DS18B20_H__