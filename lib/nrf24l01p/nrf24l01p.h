#ifndef __NRF24L01p_H__
#define __NRF24L01p_H__

#include "stm32f1xx_hal.h"
#include "string.h"
#include "NRF24L01pMemoryMap.h"


typedef enum {
    NRF24L01p_1MBPS = 0,
    NRF24L01p_2MBPS,
    NRF24L01p_250KBPS
} NRF24L01pDataRateEnum;


class NRF24L01p {

private:
    SPI_HandleTypeDef* hspi;

    GPIO_TypeDef* nrf_ce_GPIOx;
    uint16_t nrf_ce_GPIO_Pin; /* "Chip Enable" pin, activates the RX or TX role */

    GPIO_TypeDef* nrf_csn_GPIOx;
    uint16_t nrf_csn_GPIO_Pin; /* SPI Chip select */

    UART_HandleTypeDef* log_huart;

    void _ceEnable();
    void _ceDisable();
    void _csHigh();
    void _csLow();

    void _writeRegister(uint8_t reg, uint8_t data);
    void _writeRegister(uint8_t reg, uint8_t *data, uint32_t size);
    uint8_t _readRegister(uint8_t reg);
    void _readRegister(uint8_t reg, uint8_t *data, uint32_t size);
    void _sendCommand(uint8_t command);

public:
    void reset();
    void powerUp();
    void powerDown();
    void setTxMode();

    void handleSpiStatus(HAL_StatusTypeDef _status, uint8_t count);


    NRF24L01p(SPI_HandleTypeDef* _hspi, GPIO_TypeDef* _nrf_ce_GPIOx, uint16_t _nrf_ce_GPIO_Pin, GPIO_TypeDef* _nrf_csn_GPIOx, uint16_t _nrf_csn_GPIO_Pin, UART_HandleTypeDef* _huart);


    void openWritingPipe(uint64_t address, uint8_t cannel);
    bool write(uint8_t *data);
    void readAll(uint8_t *data);
    void receive(uint8_t *data);
    uint8_t isDataAvailable (int pipenum);
    void openReadingPipe(uint64_t address, uint8_t channel);
    void init();
    void printRegister(uint8_t reg);
    void printAllRegisters();
    void setRetries(uint8_t delay, uint8_t count);
    bool setDataRate(NRF24L01pDataRateEnum NRF24L01pDataRate);
    void setPayloadSize(uint8_t size);
};

#endif // __NRF24L01p_H__