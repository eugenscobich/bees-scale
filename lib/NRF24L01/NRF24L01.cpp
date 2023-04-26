
#include "Nrf24l01.h"

void Nrf24l01::ceSelect() {
    HAL_GPIO_WritePin(nrf_ce_GPIOx, nrf_ce_GPIO_Pin, GPIO_PIN_SET);
}

void Nrf24l01::ceUnselect() {
    HAL_GPIO_WritePin(nrf_ce_GPIOx, nrf_ce_GPIO_Pin, GPIO_PIN_RESET);
}

void Nrf24l01::csnSelect() {
    HAL_GPIO_WritePin(nrf_csn_GPIOx, nrf_csn_GPIO_Pin, GPIO_PIN_SET);
}

void Nrf24l01::csnUnselect() {
    HAL_GPIO_WritePin(nrf_csn_GPIOx, nrf_csn_GPIO_Pin, GPIO_PIN_RESET);
}

// Write single data
void Nrf24l01::writeRegister(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = W_REGISTER | reg;
    buf[1] = data;

    ceSelect();
    
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, buf, 2, 1000);
    handleSpiStatus(halStatus, 1);

    ceUnselect();
}

// Write multiple data
void Nrf24l01::writeRegister(uint8_t reg, uint8_t *data, uint32_t size) {
    uint8_t buf[1];
    buf[0] = W_REGISTER | reg;
    
    ceSelect();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, buf, 1, 1000);
    handleSpiStatus(halStatus, 2);
    halStatus = HAL_SPI_Transmit(hspi, data, size, 1000);
    handleSpiStatus(halStatus, 3);
    ceUnselect();
}

// Read single data
uint8_t Nrf24l01::readRegister(uint8_t reg, uint8_t data) {
    uint8_t buf[1];
    buf[0] = R_REGISTER | reg;
    ceSelect();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, buf, 1, 1000);
    handleSpiStatus(halStatus, 4);
    halStatus = HAL_SPI_Receive(hspi, &data, 1, 1000);
    handleSpiStatus(halStatus, 5);
    ceUnselect();
}

// Read multiple data
void Nrf24l01::readRegister(uint8_t reg, uint8_t *data, uint32_t size) {
    uint8_t buf[1];
    buf[0] = R_REGISTER | reg;
    ceSelect();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, buf, 1, 1000);
    handleSpiStatus(halStatus, 6);
    halStatus = HAL_SPI_Receive(hspi, data, size, 1000);
    handleSpiStatus(halStatus, 7);
    ceUnselect();
}

Nrf24l01::Nrf24l01(SPI_HandleTypeDef* _hspi, GPIO_TypeDef* _nrf_ce_GPIOx, uint16_t _nrf_ce_GPIO_Pin, GPIO_TypeDef* _nrf_csn_GPIOx, uint16_t _nrf_csn_GPIO_Pin)
    : hspi(_hspi), nrf_ce_GPIOx(_nrf_ce_GPIOx), nrf_ce_GPIO_Pin(_nrf_ce_GPIO_Pin), nrf_csn_GPIOx(_nrf_csn_GPIOx), nrf_csn_GPIO_Pin(_nrf_csn_GPIO_Pin) {
    
}



void Nrf24l01::handleSpiStatus(HAL_StatusTypeDef _status, uint8_t count) {
    if (_status != HAL_OK) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        for (size_t i = 0; i < count; i++)
        {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            if (_status == HAL_ERROR) {
                HAL_Delay(1000);
            }
            if (_status == HAL_TIMEOUT) {
                HAL_Delay(3000);
            }
            if (_status == HAL_BUSY) {
                HAL_Delay(100);
            }
        }
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(4000);
    }
}