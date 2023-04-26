
#include "NRF24L01.h"

#define _BV(b) (1UL << (b))

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)


NRF24L01::NRF24L01(SPI_HandleTypeDef* _hspi, GPIO_TypeDef* _nrf_ce_GPIOx, uint16_t _nrf_ce_GPIO_Pin, GPIO_TypeDef* _nrf_csn_GPIOx, uint16_t _nrf_csn_GPIO_Pin, UART_HandleTypeDef* _huart) :
    hspi(_hspi), 
    nrf_ce_GPIOx(_nrf_ce_GPIOx),
    nrf_ce_GPIO_Pin(_nrf_ce_GPIO_Pin),
    nrf_csn_GPIOx(_nrf_csn_GPIOx),
    nrf_csn_GPIO_Pin(_nrf_csn_GPIO_Pin), 
    log_huart(_huart) {

}

void NRF24L01::_ceEnable() {
    HAL_GPIO_WritePin(nrf_ce_GPIOx, nrf_ce_GPIO_Pin, GPIO_PIN_SET);
}

void NRF24L01::_ceDisable() {
    HAL_GPIO_WritePin(nrf_ce_GPIOx, nrf_ce_GPIO_Pin, GPIO_PIN_RESET);
}

void NRF24L01::_csHigh() {
    HAL_GPIO_WritePin(nrf_csn_GPIOx, nrf_csn_GPIO_Pin, GPIO_PIN_SET);
}

void NRF24L01::_csLow() {
    HAL_GPIO_WritePin(nrf_csn_GPIOx, nrf_csn_GPIO_Pin, GPIO_PIN_RESET);
}

// Write single data
void NRF24L01::_writeRegister(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = W_REGISTER | reg;
    buf[1] = data;


    _csLow();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, buf, 2, 1000);
    handleSpiStatus(halStatus, 1);
    _csHigh();

}

// Write multiple data
void NRF24L01::_writeRegister(uint8_t reg, uint8_t *data, uint32_t size) {
    uint8_t buf[1];
    buf[0] = W_REGISTER | reg;

    _csLow();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, buf, 1, 1000);
    handleSpiStatus(halStatus, 2);
    halStatus = HAL_SPI_Transmit(hspi, data, size, 1000);
    handleSpiStatus(halStatus, 3);
    _csHigh();
}

// Read single data
uint8_t NRF24L01::_readRegister(uint8_t reg) {
    uint8_t data = 0;
    uint8_t buf[1];
    buf[0] = R_REGISTER | reg;
    _csLow();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, buf, 1, 1000);
    handleSpiStatus(halStatus, 4);
    halStatus = HAL_SPI_Receive(hspi, &data, 1, 1000);
    handleSpiStatus(halStatus, 5);
    _csHigh();
    return data;
}

// Read multiple data
void NRF24L01::_readRegister(uint8_t reg, uint8_t *data, uint32_t size) {
    uint8_t buf[1];
    buf[0] = R_REGISTER | reg;
    _csLow();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, buf, 1, 1000);
    handleSpiStatus(halStatus, 6);
    halStatus = HAL_SPI_Receive(hspi, data, size, 1000);
    handleSpiStatus(halStatus, 7);
    _csHigh();
}


void NRF24L01::_sendCommand(uint8_t command) {
    _csLow();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, &command, 1, 1000);
    handleSpiStatus(halStatus, 8);
    _csHigh();
}

void NRF24L01::init() {
    //ceDisable();
    HAL_Delay(5);
    reset();  
    setRetries(5, 15);
    setDataRate(NRF24L01_1MBPS);
    setPayloadSize(32);
    _writeRegister(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));
    _sendCommand(FLUSH_RX);
    _sendCommand(FLUSH_TX);
    _writeRegister(CONFIG, (_BV(EN_CRC) | _BV(CRCO)));
    powerUp();
    //ceEnable();
}

void NRF24L01::setRetries(uint8_t delay, uint8_t count) {
    _writeRegister(SETUP_RETR, static_cast<uint8_t>(min(15, delay) << ARD | min(15, count)));
}

bool NRF24L01::setDataRate(NRF24L01DataRateEnum NRF24L01DataRate) {
    uint8_t rfSetup = _readRegister(RF_SETUP);

    rfSetup = static_cast<uint8_t>(rfSetup & ~(_BV(RF_DR_HIGH)));
    _writeRegister(RF_SETUP, rfSetup);

    // Verify our result
    if (_readRegister(RF_SETUP) == rfSetup) {
        return true;
    }
    return false;
}


void NRF24L01::reset() {
    // Reset pins
    _ceDisable();

    // Reset registers
    _writeRegister(CONFIG, 0x08);
    _writeRegister(EN_AA, 0x3F);
    _writeRegister(EN_RXADDR, 0x03);
    _writeRegister(SETUP_AW, 0x03);
    _writeRegister(SETUP_RETR, 0x03);
    _writeRegister(RF_CH, 0x02);
    _writeRegister(RF_SETUP, 0x07);
    _writeRegister(STATUS, 0x7E);
    _writeRegister(TX_ADDR, 0x00);
    _writeRegister(RX_PW_P0, 0x00);
    _writeRegister(RX_PW_P1, 0x00);
    _writeRegister(RX_PW_P2, 0x00);
    _writeRegister(RX_PW_P3, 0x00);
    _writeRegister(RX_PW_P4, 0x00);
    _writeRegister(RX_PW_P5, 0x00);
    _writeRegister(FIFO_STATUS, 0x11);
    _writeRegister(DYNPD, 0x00);
    _writeRegister(FEATURE, 0x00);

    uint8_t rx_addr_p0_def[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
	_writeRegister(RX_ADDR_P0, rx_addr_p0_def, 5);

    uint8_t rx_addr_p1_def[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
	_writeRegister(RX_ADDR_P1, rx_addr_p1_def, 5);
	_writeRegister(RX_ADDR_P2, 0xC3);
	_writeRegister(RX_ADDR_P3, 0xC4);
	_writeRegister(RX_ADDR_P4, 0xC5);
	_writeRegister(RX_ADDR_P5, 0xC6);

    uint8_t tx_addr_def[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
	_writeRegister(TX_ADDR, tx_addr_def, 5);


    _ceEnable();
}


void NRF24L01::powerUp() {
    uint8_t new_config = _readRegister(CONFIG);
    new_config |= 1 << 1;

    _writeRegister(CONFIG, new_config);
}

void NRF24L01::powerDown() {
    uint8_t new_config = _readRegister(CONFIG);
    new_config &= 0xFD;

    _writeRegister(CONFIG, new_config);
}

void NRF24L01::openWritingPipe(uint64_t address, uint8_t cannel) {
    _ceDisable();
    _writeRegister(RF_CH, cannel);
    _writeRegister(RX_ADDR_P0, reinterpret_cast<uint8_t*>(&address), 5);
    _writeRegister(TX_ADDR, reinterpret_cast<uint8_t*>(&address), 5);

    powerUp();
    setTxMode();
    _ceEnable();
}


void NRF24L01::setTxMode() {
    uint8_t new_config = _readRegister(CONFIG);
    new_config &= 0xFE;
    _writeRegister(CONFIG, new_config);
}

bool NRF24L01::write(uint8_t *data) {

    uint8_t cmd = W_TX_PAYLOAD;

    _csLow();
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, &cmd, 1, 1000);
    handleSpiStatus(halStatus, 8);
    halStatus = HAL_SPI_Transmit(hspi, data, 32, 1000);
    handleSpiStatus(halStatus, 9);
    _csHigh();

    HAL_Delay(1);


    uint8_t fifostatus = _readRegister(FIFO_STATUS);

	// check the fourth bit of FIFO_STATUS to know if the TX fifo is empty
	if ((fifostatus&(1<<4)) && (!(fifostatus&(1<<3))))
	{
		cmd = FLUSH_TX;
		_sendCommand(cmd);
		// reset FIFO_STATUS
		_writeRegister(FIFO_STATUS, 0x11);
		return true;
	}
    return false;
}

void NRF24L01::openReadingPipe(uint64_t address, uint8_t channel) {
	// disable the chip before configuring the device
	_ceDisable();

	_writeRegister(STATUS, 0x00);
	_writeRegister(RF_CH, channel);  // select the channel

	// select data pipe 2
	uint8_t en_rxaddr = _readRegister(EN_RXADDR);
	en_rxaddr = en_rxaddr | (1<<2);
	_writeRegister (EN_RXADDR, en_rxaddr);

	/* We must write the address for Data Pipe 1, if we want to use any pipe from 2 to 5
	 * The Address from DATA Pipe 2 to Data Pipe 5 differs only in the LSB
	 * Their 4 MSB Bytes will still be same as Data Pipe 1
	 *
	 * For Eg->
	 * Pipe 1 ADDR = 0xAABBCCDD11
	 * Pipe 2 ADDR = 0xAABBCCDD22
	 * Pipe 3 ADDR = 0xAABBCCDD33
	 *
	 */
	_writeRegister(RX_ADDR_P1, reinterpret_cast<uint8_t*>(&address), 5);  // Write the Pipe1 address
	_writeRegister(RX_ADDR_P2, 0xEE);  // Write the Pipe2 LSB address
	_writeRegister(RX_PW_P2, 0x20);   // 32 bit payload size for pipe 2


	// power up the device in Rx mode
	uint8_t config = _readRegister(CONFIG);
	config = config | (1<<1) | (1<<0);
	_writeRegister(CONFIG, config);

	// Enable the chip after configuring the device
	_ceEnable();
}


uint8_t NRF24L01::isDataAvailable (int pipenum) {
	uint8_t status = _readRegister(STATUS);
	if ((status&(1<<6))&&(status&(pipenum<<1))) {
		_writeRegister(STATUS, (1<<6));
		return 1;
	}
	return 0;
}


void NRF24L01::receive(uint8_t *data) {
	uint8_t cmdtosend = 0;

	// select the device

    _csLow();
	// payload command
	cmdtosend = R_RX_PAYLOAD;
	HAL_SPI_Transmit(hspi, &cmdtosend, 1, 1000);

	// Receive the payload
	HAL_SPI_Receive(hspi, data, 32, 1000);

	// Unselect the device

    _csHigh();

	HAL_Delay(1);

	cmdtosend = FLUSH_RX;
	_sendCommand(cmdtosend);
}



// Read all the Register data
void NRF24L01::readAll (uint8_t *data)
{
	for (int i=0; i<10; i++)
	{
		*(data+i) = _readRegister(i);
	}

	_readRegister(RX_ADDR_P0, (data+10), 5);

	_readRegister(RX_ADDR_P1, (data+15), 5);

	*(data+20) = _readRegister(RX_ADDR_P2);
	*(data+21) = _readRegister(RX_ADDR_P3);
	*(data+22) = _readRegister(RX_ADDR_P4);
	*(data+23) = _readRegister(RX_ADDR_P5);

	_readRegister(RX_ADDR_P0, (data+24), 5);

	for (int i=29; i<38; i++)
	{
		*(data+i) = _readRegister(i-12);
	}

}


void NRF24L01::setPayloadSize(uint8_t size) {
    // payload size must be in range [1, 32]
    uint8_t payload_size = static_cast<uint8_t>(max(1, min(32, size)));

    // write static payload size setting for all pipes
    for (uint8_t i = 0; i < 6; ++i) {
        _writeRegister(static_cast<uint8_t>(RX_PW_P0 + i), payload_size);
    }
}

void NRF24L01::handleSpiStatus(HAL_StatusTypeDef _status, uint8_t count) {
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

void NRF24L01::printAllRegisters() {
    printRegister(CONFIG);
    printRegister(EN_AA);
    printRegister(EN_RXADDR);
    printRegister(SETUP_AW);
    printRegister(SETUP_RETR);
    printRegister(RF_CH);
    printRegister(RF_SETUP);
    printRegister(STATUS);
    printRegister(OBSERVE_TX);
    printRegister(RPD);
    printRegister(RX_ADDR_P0);
    printRegister(RX_ADDR_P1);
    printRegister(RX_ADDR_P2);
    printRegister(RX_ADDR_P3);
    printRegister(RX_ADDR_P4);
    printRegister(RX_ADDR_P5);
    printRegister(TX_ADDR);
    printRegister(RX_PW_P0);
    printRegister(RX_PW_P1);
    printRegister(RX_PW_P2);
    printRegister(RX_PW_P3);
    printRegister(RX_PW_P4);
    printRegister(RX_PW_P5);
    printRegister(FIFO_STATUS);
    printRegister(DYNPD);
    printRegister(FEATURE);
}

void NRF24L01::printRegister(uint8_t reg) {

        uint8_t data = _readRegister(reg);

        uint8_t buf[100];
        buf[0] = 'R';
        buf[1] = 'e';
        buf[2] = 'g';
        buf[3] = '[';
        buf[4] = (reg | 0x7F) == 0xFF ? '1' : '0';
        buf[5] = (reg | 0xBF) == 0xFF ? '1' : '0';
        buf[6] = (reg | 0xDF) == 0xFF ? '1' : '0';
        buf[7] = (reg | 0xEF) == 0xFF ? '1' : '0';
        buf[8] = (reg | 0xF7) == 0xFF ? '1' : '0';
        buf[9] = (reg | 0xFB) == 0xFF ? '1' : '0';
        buf[10] = (reg | 0xFD) == 0xFF ? '1' : '0';
        buf[11] = (reg | 0xFE) == 0xFF ? '1' : '0';
        buf[12] = ']';
        buf[13] = ' ';
        buf[14] = '=';
        buf[15] = ' ';
        buf[16] = (data | 0x7F) == 0xFF ? '1' : '0';
        buf[17] = (data | 0xBF) == 0xFF ? '1' : '0';
        buf[18] = (data | 0xDF) == 0xFF ? '1' : '0';
        buf[19] = (data | 0xEF) == 0xFF ? '1' : '0';
        buf[20] = (data | 0xF7) == 0xFF ? '1' : '0';
        buf[21] = (data | 0xFB) == 0xFF ? '1' : '0';
        buf[22] = (data | 0xFD) == 0xFF ? '1' : '0';
        buf[23] = (data | 0xFE) == 0xFF ? '1' : '0';
        buf[24] = '\r';
        buf[25] = '\n';

        HAL_UART_Transmit(log_huart, buf, 26, 2000);

}