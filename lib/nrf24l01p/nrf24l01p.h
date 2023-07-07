#ifndef __NRF24L01p_H__
#define __NRF24L01p_H__

#include "stm32f1xx_hal.h"
#include "string.h"
#include "NRF24L01pMemoryMap.h"
#include <stdio.h>


#define _BV(bit) (1UL << (bit))
#define _CHECK_BIT(data, bitNumber) ((data & _BV(bitNumber)) > 0)

typedef enum {
    NRF24L01p_1MBPS = 0,
    NRF24L01p_2MBPS,
    NRF24L01p_250KBPS
} NRF24L01pDataRateEnum;

typedef enum {
    NRF24L01p_minus_18dBm = 0,
    NRF24L01p_minus_12dBm,
    NRF24L01p_minus_6dBm,
    NRF24L01p_0dBm
} NRF24L01pRxPowerEnum;

class NRF24L01p {

private:
    SPI_HandleTypeDef* hspi;

    GPIO_TypeDef* nrf_ce_GPIOx;
    uint16_t nrf_ce_GPIO_Pin; /* "Chip Enable" pin, activates the RX or TX role */

    GPIO_TypeDef* nrf_csn_GPIOx;
    uint16_t nrf_csn_GPIO_Pin; /* SPI Chip select */

    bool isCeEnabled();
    void enableCe();
    void disableCe();
    void setCsnHigh();
    void setCsnLow();

    HAL_StatusTypeDef SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);
    HAL_StatusTypeDef SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);

    void writeRegister(uint8_t reg, uint8_t data);
    void writeRegister(uint8_t reg, uint8_t *data, uint32_t size);
    uint8_t readRegister(uint8_t reg);
    void readRegister(uint8_t reg, uint8_t *data, uint32_t size);
    void sendCommand(uint8_t command);

    void readRxFifo(uint8_t *data);
    void clearStatusRxDrFlag();

    void printConfigRegister();
    void printEnableAutoAcknolageRegister();
    void printEnableRXAddressesRegister();
    void printSetupAdressWidthRegister();
    void printSetuRetransmissionRegister();
    void printRfChannelRegister();
    void printRfSetupRegister();
    void printStatusRegister();
    void printObserveTxRegister();
    void printRpdRegister();
    void printReceiveAddressDataPipesRegister();
    void printReceiveAddressDataPipe0Register();
    void printReceiveAddressDataPipe1Register();
    void printReceiveAddressDataPipe2Register();
    void printReceiveAddressDataPipe3Register();
    void printReceiveAddressDataPipe4Register();
    void printReceiveAddressDataPipe5Register();
    void printTransmitAddressRegister();
    void printReceiveNumberOfBytesInDataPipesRegister();
    void printReceiveNumberOfBytesInDataPipe0Register();
    void printReceiveNumberOfBytesInDataPipe1Register();
    void printReceiveNumberOfBytesInDataPipe2Register();
    void printReceiveNumberOfBytesInDataPipe3Register();
    void printReceiveNumberOfBytesInDataPipe4Register();
    void printReceiveNumberOfBytesInDataPipe5Register();
    void printFifoStatusRegister();
    void printEnableDynamicPayloadLenghtRegister();
    void printFeatureRegister();


public:
    void reset();
    void powerUp();
    void powerDown();
    bool isPowerUp();
    bool isPowerDown();
    bool isInTxMode();
    bool isInRxMode();

    void setTxMode();

    // Check if data is available
    bool isDataAvailable();
    // Return the pipe number where data is available
    uint8_t getDataPipeAvailable();

    void handleSpiStatus(HAL_StatusTypeDef _status, uint8_t count);


    NRF24L01p(SPI_HandleTypeDef* _hspi, GPIO_TypeDef* _nrf_ce_GPIOx, uint16_t _nrf_ce_GPIO_Pin, GPIO_TypeDef* _nrf_csn_GPIOx, uint16_t _nrf_csn_GPIO_Pin);


    void openWritingPipe(uint64_t address, uint8_t cannel);
    bool write(uint8_t *data);
    void readAll(uint8_t *data);
    void receive(uint8_t *data);
    
    void openReadingPipe(uint64_t address, uint8_t channel);
    void init();
    void printRegister(uint8_t reg);
    void printAllRegisters();
    void setRetries(uint8_t delay, uint8_t count);
    bool setDataRate(NRF24L01pDataRateEnum nrf24L01pDataRate);
    bool setRxPowerRate(NRF24L01pRxPowerEnum nrf24L01pRxPowerEnum);
    void setPayloadSize(uint8_t size);
};

#endif // __NRF24L01p_H__