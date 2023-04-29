#ifndef __NRF24L01p_H__
#define __NRF24L01p_H__

#include "stm32f1xx_hal.h"
#include "string.h"
#include "NRF24L01pMemoryMap.h"
#include <stdio.h>


#define _BV(b) (1UL << (b))
#define _CHECK_BIT(data, bit) ((data & (1UL << (bit))) > 0)

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

    void enableCE();
    void disableCE();
    void setCsnHigh();
    void setCsnLow();

    void writeRegister(uint8_t reg, uint8_t data);
    void writeRegister(uint8_t reg, uint8_t *data, uint32_t size);
    uint8_t readRegister(uint8_t reg);
    void readRegister(uint8_t reg, uint8_t *data, uint32_t size);
    void sendCommand(uint8_t command);
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

    void handleSpiStatus(HAL_StatusTypeDef _status, uint8_t count);


    NRF24L01p(SPI_HandleTypeDef* _hspi, GPIO_TypeDef* _nrf_ce_GPIOx, uint16_t _nrf_ce_GPIO_Pin, GPIO_TypeDef* _nrf_csn_GPIOx, uint16_t _nrf_csn_GPIO_Pin);


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