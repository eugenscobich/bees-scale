#ifndef __Nrf24l01_H__
#define __Nrf24l01_H__

#include "spi.h"

/* nRF24L01+ Commands */
#define R_REGISTER                  0b00000000
#define W_REGISTER                  0b00100000
#define R_RX_PAYLOAD                0b01100001
#define W_TX_PAYLOAD                0b10100000
#define FLUSH_TX                    0b11100001
#define FLUSH_RX                    0b11100010
#define REUSE_TX_PL                 0b11100011
#define R_RX_PL_WID                 0b01100000
#define W_ACK_PAYLOAD               0b10101000
#define W_TX_PAYLOAD_NOACK          0b10110000
#define NOP                         0b11111111    

/* nRF24L01+ Registers */
#define REG_CONFIG            0x00
#define REG_EN_AA             0x01
#define REG_EN_RXADDR         0x02
#define REG_SETUP_AW          0x03
#define REG_SETUP_RETR        0x04
#define REG_RF_CH             0x05
#define REG_RF_SETUP          0x06
#define REG_STATUS            0x07
#define REG_OBSERVE_TX        0x08    // Read-Only
#define REG_RPD               0x09    // Read-Only
#define REG_RX_ADDR_P0        0x0A
#define REG_RX_ADDR_P1        0x0B
#define REG_RX_ADDR_P2        0x0C
#define REG_RX_ADDR_P3        0x0D
#define REG_RX_ADDR_P4        0x0E
#define REG_RX_ADDR_P5        0x0F
#define REG_TX_ADDR           0x10
#define REG_RX_PW_P0          0x11
#define REG_RX_PW_P1          0x12
#define REG_RX_PW_P2          0x13
#define REG_RX_PW_P3          0x14
#define REG_RX_PW_P4          0x15
#define REG_RX_PW_P5          0x16
#define REG_FIFO_STATUS       0x17
#define REG_DYNPD             0x1C
#define REG_FEATURE           0x1D



class Nrf24l01 {

private:
    SPI_HandleTypeDef* hspi;

    GPIO_TypeDef* nrf_ce_GPIOx;
    uint16_t nrf_ce_GPIO_Pin; /* "Chip Enable" pin, activates the RX or TX role */

    GPIO_TypeDef* nrf_csn_GPIOx;
    uint16_t nrf_csn_GPIO_Pin; /* SPI Chip select */
 
    uint8_t address_width;

public:
    
    void ceEnable();
    void ceDisable();
    void csnSelect();
    void csnUnselect();
    void handleSpiStatus(HAL_StatusTypeDef _status, uint8_t count);

    Nrf24l01(SPI_HandleTypeDef* _hspi, GPIO_TypeDef* _nrf_ce_GPIOx, uint16_t _nrf_ce_GPIO_Pin, GPIO_TypeDef* _nrf_csn_GPIOx, uint16_t _nrf_csn_GPIO_Pin);

    void writeRegister(uint8_t reg, uint8_t data);
    void writeRegister(uint8_t reg, uint8_t *data, uint32_t size);
    uint8_t readRegister(uint8_t reg, uint8_t data);
    void readRegister(uint8_t reg, uint8_t *data, uint32_t size);
    void sendCommand(uint8_t command);
    void init();

    
};

#endif // __Nrf24l01_H__