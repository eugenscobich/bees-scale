
#include "NRF24L01p.h"
#define CLASS_NAME "NRF24L01p"
#include "log.h"


#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)

NRF24L01p::NRF24L01p(
    SPI_HandleTypeDef *_hspi, 
    GPIO_TypeDef *_nrfCeGPIOx, 
    uint16_t _nrfCeGPIOPin, 
    GPIO_TypeDef *_nrfCsnGPIOx, 
    uint16_t _nrfCsnGPIOPin, 
    void(*_updateFunction)()) : 
        hspi(_hspi),
        nrfCeGPIOx(_nrfCeGPIOx),
        nrfCeGPIOPin(_nrfCeGPIOPin),
        nrfCsnGPIOx(_nrfCsnGPIOx),
        nrfCsnGPIOPin(_nrfCsnGPIOPin),
        updateFunction(_updateFunction)
{
}

void NRF24L01p::nonBlockingDelay(uint32_t delayInTicks) {
    startDelayTick = HAL_GetTick();
    while (true) {
        if (startDelayTick + delayInTicks < HAL_GetTick()) {
            return;
        }
        updateFunction();
    }
}

HAL_StatusTypeDef NRF24L01p::SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
    HAL_StatusTypeDef halStatus = HAL_SPI_Receive(hspi, pData, Size, HAL_MAX_DELAY);
    if (halStatus == HAL_BUSY)
    {
        logError("HAL_SPI_Receive statuts: HAL_BUSY, retry after 10us\r\n");
        nonBlockingDelay(10);
        return SPI_Receive(hspi, pData, Size);
    }
    else if (halStatus == HAL_ERROR)
    {
        logError("HAL_SPI_Receive statuts: HAL_ERROR, nothing to do\r\n");
        return halStatus;
    }
    else if (halStatus == HAL_TIMEOUT)
    {
        logError("HAL_SPI_Receive statuts: HAL_TIMEOUT, nothing to do\r\n");
        return halStatus;
    }
    return halStatus;
}

HAL_StatusTypeDef NRF24L01p::SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(hspi, pData, Size, HAL_MAX_DELAY);
    if (halStatus == HAL_BUSY)
    {
        logError("HAL_SPI_Receive statuts: HAL_BUSY, retry after 10us\r\n");
        nonBlockingDelay(10);
        return SPI_Receive(hspi, pData, Size);
    }
    else if (halStatus == HAL_ERROR)
    {
        logError("HAL_SPI_Receive statuts: HAL_ERROR, nothing to do\r\n");
        return halStatus;
    }
    else if (halStatus == HAL_TIMEOUT)
    {
        logError("HAL_SPI_Receive statuts: HAL_TIMEOUT, nothing to do\r\n");
        return halStatus;
    }
    return halStatus;
}

bool NRF24L01p::isCeEnabled()
{
    return HAL_GPIO_ReadPin(nrfCeGPIOx, nrfCeGPIOPin) == GPIO_PIN_SET;
}

void NRF24L01p::enableCe()
{
    HAL_GPIO_WritePin(nrfCeGPIOx, nrfCeGPIOPin, GPIO_PIN_SET);
}

void NRF24L01p::disableCe()
{
    HAL_GPIO_WritePin(nrfCeGPIOx, nrfCeGPIOPin, GPIO_PIN_RESET);
}

void NRF24L01p::setCsnHigh()
{
    HAL_GPIO_WritePin(nrfCsnGPIOx, nrfCsnGPIOPin, GPIO_PIN_SET);
}

void NRF24L01p::setCsnLow()
{
    HAL_GPIO_WritePin(nrfCsnGPIOx, nrfCsnGPIOPin, GPIO_PIN_RESET);
}

// Write single data
void NRF24L01p::writeRegister(uint8_t reg, uint8_t data)
{
    uint8_t buf[2];
    buf[0] = W_REGISTER | reg;
    buf[1] = data;

    setCsnLow();
    SPI_Transmit(hspi, buf, 2);
    setCsnHigh();
}

// Write multiple data
void NRF24L01p::writeRegister(uint8_t reg, uint8_t *data, uint32_t size)
{
    uint8_t buf[1];
    buf[0] = W_REGISTER | reg;

    setCsnLow();
    SPI_Transmit(hspi, buf, 1);
    SPI_Transmit(hspi, data, size);
    setCsnHigh();
}

// Read single data
uint8_t NRF24L01p::readRegister(uint8_t reg)
{
    uint8_t data = 0;
    uint8_t buf[1];
    buf[0] = R_REGISTER | reg;
    setCsnLow();
    SPI_Transmit(hspi, buf, 1);
    SPI_Receive(hspi, &data, 1);
    setCsnHigh();
    return data;
}

// Read multiple data
void NRF24L01p::readRegister(uint8_t reg, uint8_t *data, uint32_t size)
{
    uint8_t buf[1];
    buf[0] = R_REGISTER | reg;
    setCsnLow();
    SPI_Transmit(hspi, buf, 1);
    SPI_Receive(hspi, data, size);
    setCsnHigh();
}

void NRF24L01p::sendCommand(uint8_t command)
{
    setCsnLow();
    SPI_Transmit(hspi, &command, 1);
    setCsnHigh();
}

/* ========================================== Public Methods ===================================================*/

// Write multiple data
void NRF24L01p::writeAcknowledgePayload(uint8_t pipeNumber, uint8_t *data, uint8_t size)
{
    uint8_t buf[1];
    buf[0] = W_ACK_PAYLOAD | (pipeNumber & 0x07);

    setCsnLow();
    SPI_Transmit(hspi, buf, 1);
    SPI_Transmit(hspi, data, size);
    setCsnHigh();
}

void NRF24L01p::flushRx()
{
    sendCommand(FLUSH_RX);
}

void NRF24L01p::flushTx()
{
    sendCommand(FLUSH_TX);
}

void NRF24L01p::init()
{
    disableCe();
    reset();
    setRetries(5, 15);
    disableIRQForMaxRetry();
    disableIRQForTx();
    setPayloadSize(RX_PIPE_0, 32);
    setPayloadSize(RX_PIPE_1, 32);
    setCRCONumberOfBytes(2);
    setChannel(72);
    powerUp();
}

void NRF24L01p::reset()
{
    // Reset registers
    writeRegister(CONFIG, 0x08);     // Enabled CRC, Reflect RX, TX and MAX Retries on IRQ
    writeRegister(EN_AA, 0x3F);      // All pipes have auto acknoladge
    writeRegister(EN_RXADDR, 0x03);  // Only pipe0 and pipe1 are enabled
    writeRegister(SETUP_AW, 0x03);   // 5 byte address
    writeRegister(SETUP_RETR, 0x03); // 250uS retransmit intervals and 3 max retranmitions
    writeRegister(RF_CH, 0x02);      // Channel nr 2
    writeRegister(RF_SETUP, 0x0E);   // 2Mbits and 0dBm
    writeRegister(STATUS, 0x70);     // Reset Status
    uint8_t rxAddress0[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    writeRegister(RX_ADDR_P0, rxAddress0, 5); // Receive Address pipe 0
    uint8_t rxAddress1[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
    writeRegister(RX_ADDR_P1, rxAddress1, 5); // Receive Address pipe 1
    writeRegister(RX_ADDR_P2, 0xC3);          // Receive Address pipe 2
    writeRegister(RX_ADDR_P3, 0xC4);          // Receive Address pipe 3
    writeRegister(RX_ADDR_P4, 0xC5);          // Receive Address pipe 4
    writeRegister(RX_ADDR_P5, 0xC6);          // Receive Address pipe 5
    writeRegister(TX_ADDR, rxAddress0, 5);    // Transmit Address
    writeRegister(RX_PW_P0, 0x00);
    writeRegister(RX_PW_P1, 0x00);
    writeRegister(RX_PW_P2, 0x00);
    writeRegister(RX_PW_P3, 0x00);
    writeRegister(RX_PW_P4, 0x00);
    writeRegister(RX_PW_P5, 0x00);
    writeRegister(DYNPD, 0x00);
    writeRegister(FEATURE, 0x00);
}

void NRF24L01p::openWritingPipe(uint64_t address)
{
    uint8_t enableRxAddreses = readRegister(EN_RXADDR);
    enableRxAddreses |= _BV(EN_RXADDR_ERX_P0_0);
    writeRegister(EN_RXADDR, enableRxAddreses);

    writeRegister(RX_ADDR_P0, reinterpret_cast<uint8_t *>(&address), 5);
    writeRegister(TX_ADDR, reinterpret_cast<uint8_t *>(&address), 5);
}

void NRF24L01p::stopListening()
{
    disableCe();
    //nonBlockingDelay(100);
    flushRx();
    setTxMode();
    clearStatus();
}

void NRF24L01p::openReadingPipe(uint64_t address, uint8_t pipeNumber)
{
    uint8_t enableRxAddreses = readRegister(EN_RXADDR);
    enableRxAddreses |= _BV(EN_RXADDR_ERX_P0_0 + max(0, min(5, pipeNumber)));
    writeRegister(EN_RXADDR, enableRxAddreses);

    if (pipeNumber < 2) {
        writeRegister(RX_ADDR_P0 + max(0, min(5, pipeNumber)), reinterpret_cast<uint8_t *>(&address), 5); // Write the pipe address   
    }
    else 
    {
        writeRegister(RX_ADDR_P0 + max(0, min(5, pipeNumber - 1)), reinterpret_cast<uint8_t *>(&address), 5); // Write the pipe address   
        writeRegister(RX_ADDR_P0 + max(0, min(5, pipeNumber)), reinterpret_cast<uint8_t *>(&address + 4), 1); // Write the pipe address   
    }    
}

void NRF24L01p::startListening()
{
    setRxMode();
    clearStatus();
    // enable listining
    enableCe();
    // continue to wait for data avalability
    disablePipe(RX_PIPE_0);
}

bool NRF24L01p::write(uint8_t *data)
{
    flushTx();
    writeTxFifo(data);
    nonBlockingDelay(1);
    // start sending
    enableCe();
    uint32_t timer = HAL_GetTick();
    while (true)
    {
        uint8_t status = readRegister(STATUS);
        if (_CHECK_BIT(status, STATUS_TX_DS_5))
        {
            clearTxStatus();
            return true;
        }

        if (_CHECK_BIT(status, STATUS_MAX_RT_4) || (timer + 100) < HAL_GetTick())
        {
            flushTx();
            clearMaxRtStatus();
            return false;
        }
    }
    return false;
}

bool NRF24L01p::isDataReceived()
{
    uint8_t status = readRegister(STATUS);
    return _CHECK_BIT(status, STATUS_RX_DR_6);
}

bool NRF24L01p::isDataAvailable()
{
    uint8_t status = readRegister(STATUS);
    uint8_t pipeNumber = status >> STATUS_RX_P_NO_1 & 0x07;
    return pipeNumber <= 5;
}

void NRF24L01p::receive(uint8_t *data)
{
    readRxFifo(data);
    clearRxStatus();
    flushRx();
}

void NRF24L01p::readRxFifo(uint8_t *data)
{
    uint8_t cmd = R_RX_PAYLOAD;
    setCsnLow();
    SPI_Transmit(hspi, &cmd, 1);
    SPI_Receive(hspi, data, 32);
    setCsnHigh();
}

void NRF24L01p::writeTxFifo(uint8_t *data) {
    uint8_t cmd = W_TX_PAYLOAD;
    setCsnLow();
    SPI_Transmit(hspi, &cmd, 1);
    SPI_Transmit(hspi, data, 32);
    setCsnHigh();
}

// =====================================================================

void NRF24L01p::enableDynamicPayload(uint8_t pipeNumber)
{
    uint8_t feature = readRegister(FEATURE);
    feature |= _BV(FEATURE_EN_DPL_2);
    writeRegister(FEATURE, feature);
    if (readRegister(FEATURE) != feature)
    {
        logError("Could not enable dynamic payload\r\n");
    }

    uint8_t dynpd = readRegister(DYNPD);
    dynpd |= _BV(DYNPD_DPL_P0_0) << max(0, min(5, pipeNumber));
    writeRegister(DYNPD, dynpd);
    if (readRegister(DYNPD) != dynpd)
    {
        logError("Could not set dynamic payload for pipe %d\r\n", pipeNumber);
    }
}

void NRF24L01p::enablePayloadWithAknoladge()
{
    uint8_t feature = readRegister(FEATURE);
    feature |= _BV(FEATURE_EN_ACK_PAY_1);
    writeRegister(FEATURE, feature);
    if (readRegister(FEATURE) != feature)
    {
        logError("Could not enable dynamic payload\r\n");
    }
}

void NRF24L01p::disablePipe(uint8_t pipeNumber)
{
    int8_t addreses = readRegister(EN_RXADDR);
    addreses &= ~(_BV(EN_RXADDR_ERX_P0_0) << max(0, min(5, pipeNumber)));
    writeRegister(EN_RXADDR, addreses);
    if (readRegister(EN_RXADDR) != addreses)
    {
        logError("Could not disable pipe: %d\r\n", addreses);
    }
}

void NRF24L01p::clearRxStatus()
{
    int8_t status = readRegister(STATUS);
    status |= _BV(STATUS_RX_DR_6);
    writeRegister(STATUS, status);
}
void NRF24L01p::clearTxStatus()
{
    int8_t status = readRegister(STATUS);
    status |= _BV(STATUS_TX_DS_5);
    writeRegister(STATUS, status);
}
void NRF24L01p::clearMaxRtStatus()
{
    int8_t status = readRegister(STATUS);
    status |= _BV(STATUS_MAX_RT_4);
    writeRegister(STATUS, status);
}

void NRF24L01p::clearStatus()
{
    int8_t status = readRegister(STATUS);
    status |= (_BV(STATUS_RX_DR_6) | _BV(STATUS_TX_DS_5) | _BV(STATUS_MAX_RT_4));
    writeRegister(STATUS, status);
}

bool NRF24L01p::isPowerUp()
{
    int8_t data = readRegister(CONFIG);
    return _CHECK_BIT(data, CONFIG_PWR_UP_1);
}

bool NRF24L01p::isPowerDown()
{
    return !isPowerUp();
}

bool NRF24L01p::isInRxMode()
{
    uint8_t data = readRegister(CONFIG);
    return _CHECK_BIT(data, CONFIG_PRIM_RX_0) && isCeEnabled();
}

bool NRF24L01p::isInTxMode()
{
    return !isInRxMode();
}

void NRF24L01p::setRetries(uint8_t delay, uint8_t count)
{
    writeRegister(SETUP_RETR, static_cast<uint8_t>(min(15, delay) << 4 | min(15, count)));
}

void NRF24L01p::setDataRate(NRF24L01pDataRateEnum nrf24L01pDataRate)
{
    uint8_t rfSetup = readRegister(RF_SETUP);
    rfSetup = static_cast<uint8_t>(rfSetup & ~(_BV(RF_SETUP_RF_DR_LOW_5)));  // remove bit
    rfSetup = static_cast<uint8_t>(rfSetup & ~(_BV(RF_SETUP_RF_DR_HIGH_3))); // remove bit
    if (nrf24L01pDataRate == NRF24L01p_250KBPS)
    {
        rfSetup = static_cast<uint8_t>(rfSetup | (_BV(RF_SETUP_RF_DR_LOW_5))); // set bit
    }
    else if (nrf24L01pDataRate == NRF24L01p_2MBPS)
    {
        rfSetup = static_cast<uint8_t>(rfSetup | (_BV(RF_SETUP_RF_DR_HIGH_3)));
    }

    writeRegister(RF_SETUP, rfSetup);

    if (readRegister(RF_SETUP) != rfSetup)
    {
        logError("Could not data rate: %d\r\n", nrf24L01pDataRate);
    }
}

void NRF24L01p::setRxPowerRate(NRF24L01pRxPowerEnum nrf24L01pRxPowerEnum)
{
    uint8_t rfSetup = readRegister(RF_SETUP);
    rfSetup = static_cast<uint8_t>(rfSetup & ~(_BV(RF_SETUP_RF_PWR_2))); // remove bit
    rfSetup = static_cast<uint8_t>(rfSetup & ~(_BV(RF_SETUP_RF_PWR_1))); // remove bit
    if (nrf24L01pRxPowerEnum == NRF24L01p_0dBm)
    {
        rfSetup = static_cast<uint8_t>(rfSetup | _BV(RF_SETUP_RF_PWR_2) | _BV(RF_SETUP_RF_PWR_1)); // set bit
    }
    else if (nrf24L01pRxPowerEnum == NRF24L01p_minus_6dBm)
    {
        rfSetup = static_cast<uint8_t>(rfSetup | _BV(RF_SETUP_RF_PWR_2));
    }
    else if (nrf24L01pRxPowerEnum == NRF24L01p_minus_12dBm)
    {
        rfSetup = static_cast<uint8_t>(rfSetup | _BV(RF_SETUP_RF_PWR_1));
    }

    writeRegister(RF_SETUP, rfSetup);

    if (readRegister(RF_SETUP) != rfSetup)
    {
        logError("Could not power rate: %d\r\n", nrf24L01pRxPowerEnum);
    }
}

void NRF24L01p::powerUp()
{
    uint8_t config = readRegister(CONFIG);
    if (!_CHECK_BIT(config, CONFIG_PWR_UP_1))
    {
        config |= _BV(CONFIG_PWR_UP_1);
        writeRegister(CONFIG, config);
        HAL_Delay(10);
        if (readRegister(CONFIG) != config)
        {
            logError("Could not power up\r\n");
        }
    }
}

void NRF24L01p::powerDown()
{
    disableCe();
    uint8_t config = readRegister(CONFIG);
    config &= ~(_BV(CONFIG_PWR_UP_1));
    writeRegister(CONFIG, config);
    if (readRegister(CONFIG) != config)
    {
        logError("Could not power down\r\n");
    }
}

void NRF24L01p::setTxMode()
{
    uint8_t config = readRegister(CONFIG);
    config &= ~(_BV(CONFIG_PRIM_RX_0));
    writeRegister(CONFIG, config);
    if (readRegister(CONFIG) != config)
    {
        logError("Could not set in TX Mode\r\n");
    }
}

void NRF24L01p::setRxMode()
{
    uint8_t config = readRegister(CONFIG);
    config |= _BV(CONFIG_PRIM_RX_0);
    writeRegister(CONFIG, config);
    if (readRegister(CONFIG) != config)
    {
        logError("Could not set in RX Mode\r\n");
    }
}

void NRF24L01p::setChannel(uint8_t channel)
{
    writeRegister(RF_CH, channel);
    if (readRegister(RF_CH) != channel)
    {
        logError("Could not set the Channel: %d\r\n", channel);
    }
}

uint8_t NRF24L01p::getDataPipeAvailable()
{
    uint8_t data = readRegister(STATUS);
    return (data & (_BV(STATUS_RX_P_NO_3) | _BV(STATUS_RX_P_NO_2) | _BV(STATUS_RX_P_NO_1)) >> 1);
}

// Read all the Register data
void NRF24L01p::readAll(uint8_t *data)
{
    for (int i = 0; i < 10; i++)
    {
        *(data + i) = readRegister(i);
    }

    readRegister(RX_ADDR_P0, (data + 10), 5);

    readRegister(RX_ADDR_P1, (data + 15), 5);

    *(data + 20) = readRegister(RX_ADDR_P2);
    *(data + 21) = readRegister(RX_ADDR_P3);
    *(data + 22) = readRegister(RX_ADDR_P4);
    *(data + 23) = readRegister(RX_ADDR_P5);

    readRegister(RX_ADDR_P0, (data + 24), 5);

    for (int i = 29; i < 38; i++)
    {
        *(data + i) = readRegister(i - 12);
    }
}

void NRF24L01p::setPayloadSize(uint8_t pipeNumber, uint8_t size)
{
    // payload size must be in range [1, 32]
    uint8_t payloadSize = static_cast<uint8_t>(max(1, min(32, size)));
    writeRegister(RX_PW_P0 + max(0, min(5, pipeNumber)), payloadSize);
    if (readRegister(RX_PW_P0 + max(0, min(5, pipeNumber))) != payloadSize)
    {
        logError("Could not set payload\r\n");
    }
}

void NRF24L01p::setCRCONumberOfBytes(uint8_t numberOfBytes)
{
    uint8_t config = readRegister(CONFIG);
    if (max(1, min(2, numberOfBytes)) == 1)
    {
        config &= ~_BV(CONFIG_CRCO_2);
    }
    else
    {
        config |= _BV(CONFIG_CRCO_2);
    }
    writeRegister(CONFIG, config);
    if (readRegister(CONFIG) != config)
    {
        logError("Could not set CRCO\r\n");
    }
}

void NRF24L01p::disableIRQForTx()
{
    uint8_t data = readRegister(CONFIG);
    data |= _BV(CONFIG_MASK_TX_DS_5);
    writeRegister(CONFIG, data);
}

void NRF24L01p::disableIRQForRx()
{
    uint8_t data = readRegister(CONFIG);
    data |= _BV(CONFIG_MASK_RX_DR_6);
    writeRegister(CONFIG, data);
}

void NRF24L01p::disableIRQForMaxRetry()
{
    uint8_t data = readRegister(CONFIG);
    data |= _BV(CONFIG_MASK_MAX_RT_4);
    writeRegister(CONFIG, data);
}

void NRF24L01p::enableIRQForTx()
{
    uint8_t data = readRegister(CONFIG);
    data &= ~(_BV(CONFIG_MASK_TX_DS_5));
    writeRegister(CONFIG, data);
}

void NRF24L01p::enableIRQForRx()
{
    uint8_t data = readRegister(CONFIG);
    data &= ~(_BV(CONFIG_MASK_RX_DR_6));
    writeRegister(CONFIG, data);
}

void NRF24L01p::enableIRQForMaxRetry()
{
    uint8_t data = readRegister(CONFIG);
    data &= ~(_BV(CONFIG_MASK_MAX_RT_4));
    writeRegister(CONFIG, data);
}

/* ================================== PRINT METHODS ===================================*/

void NRF24L01p::printAllRegisters()
{
    printCE();
    printConfigRegister();
    printEnableAutoAcknolageRegister();
    printEnableRXAddressesRegister();
    printSetupAdressWidthRegister();
    printSetuRetransmissionRegister();
    printRfChannelRegister();
    printRfSetupRegister();
    printStatusRegister();

    printObserveTxRegister();
    printRpdRegister();

    printReceiveAddressDataPipesRegister();
    printTransmitAddressRegister();
    printReceiveNumberOfBytesInDataPipesRegister();
    printFifoStatusRegister();
    printEnableDynamicPayloadLenghtRegister();
    printFeatureRegister();
}

void NRF24L01p::printCE()
{
    logDebug("Is chip enabled: %s\r\n", isCeEnabled() ? "Yes" : "No");
}

void NRF24L01p::printRegister(uint8_t reg)
{
    uint8_t data = readRegister(reg);
    char buf[27];
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
    buf[26] = '\0';
    printf(buf);
}

void NRF24L01p::printConfigRegister()
{
    uint8_t data = readRegister(CONFIG);
    char regId[] = "[0x00]";
    logDebug("%s Register [CONFIG] Configuration Register:\r\n", regId);
    logDebug("%s Mask interrupt caused by RX_DR [MASK_RX_DR]: ", regId);
    if (_CHECK_BIT(data, CONFIG_MASK_RX_DR_6))
    {
        printf("[1] Interrupt not reflected on the IRQ pin\r\n");
    }
    else
    {
        printf("[0] Reflect RX_DR as active low interrupt on the IRQ pin\r\n");
    }
    logDebug("%s Mask interrupt caused by TX_DS [MASK_TX_DR]: ", regId);
    if (_CHECK_BIT(data, CONFIG_MASK_TX_DS_5))
    {
        printf("[1] Interrupt not reflected on the IRQ pin\r\n");
    }
    else
    {
        printf("[0] Reflect TX_DR as active low interrupt on the IRQ pin\r\n");
    }
    logDebug("%s Mask interrupt caused by MAX_RT [MASK_MAX_RT]: ", regId);
    if (_CHECK_BIT(data, CONFIG_MASK_MAX_RT_4))
    {
        printf("[1] Interrupt not reflected on the IRQ pin\r\n");
    }
    else
    {
        printf("[0] Reflect MAX_RT as active low interrupt on the IRQ pin\r\n");
    }
    logDebug("%s Enable CRC [EN_CRC]: ", regId);
    if (_CHECK_BIT(data, CONFIG_EN_CRC_3))
    {
        printf("[1] Enabled or it was forced high if one of the bits in the EN_AA is high\r\n");
    }
    else
    {
        printf("[0] Disabled\r\n");
    }
    logDebug("%s CRC encoding scheme [CRCO]: ", regId);
    if (_CHECK_BIT(data, CONFIG_CRCO_2))
    {
        printf("[1] 2 byte\r\n");
    }
    else
    {
        printf("[0] 1 byte\r\n");
    }
    logDebug("%s Power [PWR_UP]: ", regId);
    if (_CHECK_BIT(data, CONFIG_PWR_UP_1))
    {
        printf("[1] POWER UP\r\n");
    }
    else
    {
        printf("[0] POWER DOWN\r\n");
    }
    logDebug("%s RX/TX control [PRIM_RX]: ", regId);
    if (_CHECK_BIT(data, CONFIG_PRIM_RX_0))
    {
        printf("[1] PRX\r\n");
    }
    else
    {
        printf("[0] PTX\r\n");
    }
}

void NRF24L01p::printEnableAutoAcknolageRegister()
{
    uint8_t data = readRegister(EN_AA);
    logDebug("[0x01] Register [EN_AA] Enable 'Auto Acknowledgment' Function:\r\n");
    char message[] = "[0x01] Enable auto acknowledgement data pipe %d [ENAA_P%d]: %s";
    char enabled[] = "[1] Enabled\r\n";
    char disabled[] = "[0] Disabled\r\n";
    logDebug(message, 5, 5, _CHECK_BIT(data, EN_AA_ENAA_P5_5) ? enabled : disabled);
    logDebug(message, 4, 4, _CHECK_BIT(data, EN_AA_ENAA_P4_4) ? enabled : disabled);
    logDebug(message, 3, 3, _CHECK_BIT(data, EN_AA_ENAA_P3_3) ? enabled : disabled);
    logDebug(message, 2, 2, _CHECK_BIT(data, EN_AA_ENAA_P2_2) ? enabled : disabled);
    logDebug(message, 1, 1, _CHECK_BIT(data, EN_AA_ENAA_P1_1) ? enabled : disabled);
    logDebug(message, 0, 0, _CHECK_BIT(data, EN_AA_ENAA_P0_0) ? enabled : disabled);
}

void NRF24L01p::printEnableRXAddressesRegister()
{
    uint8_t data = readRegister(EN_RXADDR);
    logDebug("[0x02] Register [EN_RXADDR] Enabled RX Addresses:\r\n");
    char message[] = "[0x02] Enable data pipe %d [ERX_P%d]: %s";
    char enabled[] = "[1] Enabled\r\n";
    char disabled[] = "[0] Disabled\r\n";
    logDebug(message, 5, 5, _CHECK_BIT(data, EN_RXADDR_ERX_P5_5) ? enabled : disabled);
    logDebug(message, 4, 4, _CHECK_BIT(data, EN_RXADDR_ERX_P4_4) ? enabled : disabled);
    logDebug(message, 3, 3, _CHECK_BIT(data, EN_RXADDR_ERX_P3_3) ? enabled : disabled);
    logDebug(message, 2, 2, _CHECK_BIT(data, EN_RXADDR_ERX_P2_2) ? enabled : disabled);
    logDebug(message, 1, 1, _CHECK_BIT(data, EN_RXADDR_ERX_P1_1) ? enabled : disabled);
    logDebug(message, 0, 0, _CHECK_BIT(data, EN_RXADDR_ERX_P0_0) ? enabled : disabled);
}

void NRF24L01p::printSetupAdressWidthRegister()
{
    uint8_t data = readRegister(SETUP_AW);
    logDebug("[0x03] Register [SETUP_AW] Setup of Address Widths:\r\n");
    logDebug("[0x03] RX/TX Address field width [AW]: ");
    if (_CHECK_BIT(data, SETUP_AW_AW_1))
    {
        if (_CHECK_BIT(data, SETUP_AW_AW_0))
        {
            printf("[11] 5 bytes\r\n");
        }
        else
        {
            printf("[10] 4 bytes\r\n");
        }
    }
    else
    {
        if (_CHECK_BIT(data, SETUP_AW_AW_0))
        {
            printf("[01] 3 bytes\r\n");
        }
        else
        {
            printf("[00] Illigal\r\n");
        }
    }
}

void NRF24L01p::printSetuRetransmissionRegister()
{
    uint8_t data = readRegister(SETUP_RETR);
    logDebug("[0x04] Register [SETUP_RETR] Setup of Automatic Retransmission:\r\n");
    logDebug("[0x04] Auto Retransmit Delay [ARD]: ");
    uint8_t ard = (data & (_BV(SETUP_RETR_ARD_7) | _BV(SETUP_RETR_ARD_6) | _BV(SETUP_RETR_ARD_5) | _BV(SETUP_RETR_ARD_4))) >> 4;
    printf("[%c%c%c%c] %duS\r\n",
           _CHECK_BIT(ard, 3) ? '1' : '0',
           _CHECK_BIT(ard, 2) ? '1' : '0',
           _CHECK_BIT(ard, 1) ? '1' : '0',
           _CHECK_BIT(ard, 0) ? '1' : '0',
           250 + ard * 250);

    logDebug("[0x04] Auto Retransmit Count [ARC]: ");
    uint8_t arc = (data & (_BV(SETUP_RETR_ARC_3) | _BV(SETUP_RETR_ARC_2) | _BV(SETUP_RETR_ARC_1) | _BV(SETUP_RETR_ARC_0)));
    printf("[%c%c%c%c] ",
           _CHECK_BIT(arc, 3) ? '1' : '0',
           _CHECK_BIT(arc, 2) ? '1' : '0',
           _CHECK_BIT(arc, 1) ? '1' : '0',
           _CHECK_BIT(arc, 0) ? '1' : '0');
    if (arc == 0)
    {
        printf("Re-Transmit disabled\r\n");
    }
    else
    {
        printf("Up to %d Re-Transmit on fail of auto acknoladge\r\n", arc);
    }
}

void NRF24L01p::printRfChannelRegister()
{
    uint8_t data = readRegister(RF_CH);
    logDebug("[0x05] Register [RF_CH] RF Channel: %d\r\n", data);
}

void NRF24L01p::printRfSetupRegister()
{
    uint8_t data = readRegister(RF_SETUP);
    char regId[] = "[0x06]";
    logDebug("%s Register [RF_SETUP] RF Setup Register:\r\n", regId);
    char enabled[] = "[1] Enabled\r\n";
    char disabled[] = "[0] Disabled\r\n";

    logDebug("%s Enables continuous carrier transmit when high [CONT_WAVE]: ", regId);
    printf("%s", _CHECK_BIT(data, RF_SETUP_CONT_WAVE_7) ? enabled : disabled);

    logDebug("%s Set RF Data Rate to 250kbps [RF_DR_LOW]: ", regId);
    printf("%s", _CHECK_BIT(data, RF_SETUP_RF_DR_LOW_5) ? "[1] 250kbps\r\n" : disabled);

    logDebug("%s Force PLL lock signal. Only used in test [PLL_LOCK]: ", regId);
    printf("%s", _CHECK_BIT(data, RF_SETUP_PLL_LOCK_4) ? enabled : disabled);

    logDebug("%s Select between the high speed data rates. This bit is don't care if RF_DR_LOW is set [RF_DR_HIGH]: ", regId);
    printf("%s", _CHECK_BIT(data, RF_SETUP_RF_DR_HIGH_3) ? "[1] 2Mbps\r\n" : "[0] 1Mbps\r\n");

    logDebug("%s Set RF output power in TX mode [RF_PWR]: ", regId);

    if (_CHECK_BIT(data, RF_SETUP_RF_PWR_2))
    {
        if (_CHECK_BIT(data, RF_SETUP_RF_PWR_1))
        {
            printf("[11] 0dBm\r\n");
        }
        else
        {
            printf("[10] -6dBm\r\n");
        }
    }
    else
    {
        if (_CHECK_BIT(data, RF_SETUP_RF_PWR_1))
        {
            printf("[01] -12dBm\r\n");
        }
        else
        {
            printf("[00] -18dBm\r\n");
        }
    }
}

void NRF24L01p::printStatusRegister()
{
    uint8_t data = readRegister(STATUS);
    char regId[] = "[0x07]";

    logDebug("%s Register [STATUS] Status Register:\r\n", regId);

    logDebug("%s Data Ready RX FIFO interrupt. Asserted when new data arrives RX FIFO [RX_DR]: ", regId);
    printf("%s", _CHECK_BIT(data, STATUS_RX_DR_6) ? "[1] data is ready" : "[0] no data");
    printf("\r\n");

    logDebug("%s Data Ready TX FIFO interrupt. Asserted when packet transmitted on TX [TX_DS]: ", regId);
    printf("%s", _CHECK_BIT(data, STATUS_TX_DS_5) ? "[1] data is sent" : "[0] data is not sent");
    printf("\r\n");

    logDebug("%s Maximum number of TX retransmits interrupt [MAX_RT]: ", regId);
    printf("%s", _CHECK_BIT(data, STATUS_MAX_RT_4) ? "[1] max nr of retransmits reached" : "[0] not reatched the max nr of retransmits");
    printf("\r\n");

    logDebug("%s Data pipe number for the payload available for reading from RX_FIFO [RX_P_NO]: ", regId);
    uint8_t rxPipeNumber = (data & (_BV(STATUS_RX_P_NO_3) | _BV(STATUS_RX_P_NO_2) | _BV(STATUS_RX_P_NO_1)) >> 1);
    printf("[%c%c%c] ",
           _CHECK_BIT(rxPipeNumber, 2) ? '1' : '0',
           _CHECK_BIT(rxPipeNumber, 1) ? '1' : '0',
           _CHECK_BIT(rxPipeNumber, 0) ? '1' : '0');
    if (rxPipeNumber < 5)
    {
        printf("%d", rxPipeNumber);
    }
    else if (rxPipeNumber == 7)
    {
        printf("RX FIFO Empty");
    }
    else
    {
        printf("Not Used");
    }
    printf("\r\n");

    logDebug("%s TX FIFO full flag [TX_FULL]: ", regId);
    printf("%s", _CHECK_BIT(data, STATUS_TX_FULL_0) ? "[1] TX FIFO is full" : "[0] Available locations in TX FIFO");
    printf("\r\n");
}

void NRF24L01p::printObserveTxRegister()
{
    uint8_t data = readRegister(OBSERVE_TX);
    char regId[] = "[0x08]";
    logDebug("%s Register [OBSERVE_TX] Transmit observe register:\r\n", regId);

    logDebug("%s Count lost packets [PLOS_CNT]: ", regId);
    uint8_t plosCnt = (data & (_BV(OBSERVE_TX_PLOS_CNT_7) | _BV(OBSERVE_TX_PLOS_CNT_6) | _BV(OBSERVE_TX_PLOS_CNT_5) | _BV(OBSERVE_TX_PLOS_CNT_4))) >> 4;
    printf("%d\r\n", plosCnt);

    logDebug("%s Count retransmitted packets [ARC_CNT]: ", regId);
    uint8_t txArcCnt = (data & (_BV(OBSERVE_TX_ARC_CNT_3) | _BV(OBSERVE_TX_ARC_CNT_2) | _BV(OBSERVE_TX_ARC_CNT_1) | _BV(OBSERVE_TX_ARC_CNT_0)));
    printf("%d\r\n", txArcCnt);
}

void NRF24L01p::printRpdRegister()
{
    uint8_t data = readRegister(RPD);
    logDebug("[0x09] Register [RPD]: %s\r\n", _CHECK_BIT(data, RPD_RPD) ? "[1] the received power is more than -64 dBm" : "[0] received power is less than -64 dBm");
}

void NRF24L01p::printReceiveAddressDataPipesRegister()
{
    printReceiveAddressDataPipe0Register();
    printReceiveAddressDataPipe1Register();
    printReceiveAddressDataPipe2Register();
    printReceiveAddressDataPipe3Register();
    printReceiveAddressDataPipe4Register();
    printReceiveAddressDataPipe5Register();
}

void NRF24L01p::printReceiveAddressDataPipe0Register()
{
    uint8_t data[5];
    readRegister(RX_ADDR_P0, data, 5);
    logDebug("[0x0A] Receive address data pipe 0 [RX_ADDR_P0]: 0x%02X%02X%02X%02X%02X\r\n", data[4], data[3], data[2], data[1], data[0]);
}

void NRF24L01p::printReceiveAddressDataPipe1Register()
{
    uint8_t data[5];
    readRegister(RX_ADDR_P1, data, 5);
    logDebug("[0x0B] Receive address data pipe 1 [RX_ADDR_P1]: 0x%02X%02X%02X%02X%02X\r\n", data[4], data[3], data[2], data[1], data[0]);
}

void NRF24L01p::printReceiveAddressDataPipe2Register()
{
    uint8_t data[5];
    readRegister(RX_ADDR_P1, data, 5);
    uint8_t data2 = readRegister(RX_ADDR_P2);
    logDebug("[0x0C] Receive address data pipe 2 [RX_ADDR_P2]: 0x%02X%02X%02X%02X%02X\r\n", data[4], data[3], data[2], data[1], data2);
}

void NRF24L01p::printReceiveAddressDataPipe3Register()
{
    uint8_t data[5];
    readRegister(RX_ADDR_P1, data, 5);
    uint8_t data2 = readRegister(RX_ADDR_P3);
    logDebug("[0x0D] Receive address data pipe 3 [RX_ADDR_P3]: 0x%02X%02X%02X%02X%02X\r\n", data[4], data[3], data[2], data[1], data2);
}

void NRF24L01p::printReceiveAddressDataPipe4Register()
{
    uint8_t data[5];
    readRegister(RX_ADDR_P1, data, 5);
    uint8_t data2 = readRegister(RX_ADDR_P4);
    logDebug("[0x0E] Receive address data pipe 4 [RX_ADDR_P4]: 0x%02X%02X%02X%02X%02X\r\n", data[4], data[3], data[2], data[1], data2);
}

void NRF24L01p::printReceiveAddressDataPipe5Register()
{
    uint8_t data[5];
    readRegister(RX_ADDR_P1, data, 5);
    uint8_t data2 = readRegister(RX_ADDR_P5);
    logDebug("[0x0E] Receive address data pipe 5 [RX_ADDR_P5]: 0x%02X%02X%02X%02X%02X\r\n", data[4], data[3], data[2], data[1], data2);
}

void NRF24L01p::printTransmitAddressRegister()
{
    uint8_t data[5];
    readRegister(TX_ADDR, data, 5);
    logDebug("[0x10] Register [TX_ADDR] Transmit address: 0x%02X%02X%02X%02X%02X\r\n", data[4], data[3], data[2], data[1], data[0]);
}

void NRF24L01p::printReceiveNumberOfBytesInDataPipesRegister()
{
    printReceiveNumberOfBytesInDataPipe0Register();
    printReceiveNumberOfBytesInDataPipe1Register();
    printReceiveNumberOfBytesInDataPipe2Register();
    printReceiveNumberOfBytesInDataPipe3Register();
    printReceiveNumberOfBytesInDataPipe4Register();
    printReceiveNumberOfBytesInDataPipe5Register();
}

void NRF24L01p::printReceiveNumberOfBytesInDataPipe0Register()
{
    uint8_t data = readRegister(RX_PW_P0);
    logDebug("[0x11] Number of bytes in RX payload in data pipe 0 [RX_PW_P0]: %d\r\n", data);
}

void NRF24L01p::printReceiveNumberOfBytesInDataPipe1Register()
{
    uint8_t data = readRegister(RX_PW_P1);
    logDebug("[0x12] Number of bytes in RX payload in data pipe 1 [RX_PW_P1]: %d\r\n", data);
}

void NRF24L01p::printReceiveNumberOfBytesInDataPipe2Register()
{
    uint8_t data = readRegister(RX_PW_P2);
    logDebug("[0x13] Number of bytes in RX payload in data pipe 2 [RX_PW_P2]: %d\r\n", data);
}

void NRF24L01p::printReceiveNumberOfBytesInDataPipe3Register()
{
    uint8_t data = readRegister(RX_PW_P3);
    logDebug("[0x14] Number of bytes in RX payload in data pipe 3 [RX_PW_P3]: %d\r\n", data);
}

void NRF24L01p::printReceiveNumberOfBytesInDataPipe4Register()
{
    uint8_t data = readRegister(RX_PW_P4);
    logDebug("[0x15] Number of bytes in RX payload in data pipe 4 [RX_PW_P4]: %d\r\n", data);
}

void NRF24L01p::printReceiveNumberOfBytesInDataPipe5Register()
{
    uint8_t data = readRegister(RX_PW_P5);
    logDebug("[0x16] Number of bytes in RX payload in data pipe 5 [RX_PW_P5]: %d\r\n", data);
}

void NRF24L01p::printFifoStatusRegister()
{
    uint8_t data = readRegister(FIFO_STATUS);
    char regId[] = "[0x17]";

    logDebug("%s Register [FIFO_STATUS] FIFO Status Register:\r\n", regId);

    logDebug("%s TX payload reuse is active until W_TX_PAYLOAD or FLUSH TX is executed [TX_REUSE]: ", regId);
    printf("%s", _CHECK_BIT(data, FIFO_STATUS_TX_REUSE_6) ? "[1] active" : "[0] not active");
    printf("\r\n");

    logDebug("%s TX FIFO full flag [FIFO_FULL]: ", regId);
    printf("%s", _CHECK_BIT(data, FIFO_STATUS_FIFO_FULL_5) ? "[1] TX FIFO full" : "[0] Available locations in TX FIFO");
    printf("\r\n");

    logDebug("%s TX FIFO empty flag [TX_EMPTY]: ", regId);
    printf("%s", _CHECK_BIT(data, FIFO_STATUS_TX_EMPTY_4) ? "[1] TX FIFO empty" : "[0] Data in TX FIFO");
    printf("\r\n");

    logDebug("%s RX FIFO full flag [RX_FULL]: ", regId);
    printf("%s", _CHECK_BIT(data, FIFO_STATUS_RX_FULL_1) ? "[1] RX FIFO full" : "[0] Available locations in RX FIFO");
    printf("\r\n");

    logDebug("%s RX FIFO empty flag [RX_EMPTY]: ", regId);
    printf("%s", _CHECK_BIT(data, FIFO_STATUS_RX_EMPTY_0) ? "[1] RX FIFO empty" : "[0] Data in RX FIFO");
    printf("\r\n");
}

void NRF24L01p::printEnableDynamicPayloadLenghtRegister()
{
    uint8_t data = readRegister(DYNPD);
    char regId[] = "[0x1C]";
    logDebug("%s Register [DYNPD] Enable dynamic payload length: \r\n", regId);
    char enabled[] = "[1] Enabled\r\n";
    char disabled[] = "[0] Disabled\r\n";

    logDebug("%s Enable dynamic payload length data pipe 5 [DPL_P5]: ", regId);
    printf("%s", _CHECK_BIT(data, DYNPD_DPL_P5_5) ? enabled : disabled);

    logDebug("%s Enable dynamic payload length data pipe 4 [DPL_P4]: ", regId);
    printf("%s", _CHECK_BIT(data, DYNPD_DPL_P4_4) ? enabled : disabled);

    logDebug("%s Enable dynamic payload length data pipe 3 [DPL_P3]: ", regId);
    printf("%s", _CHECK_BIT(data, DYNPD_DPL_P3_3) ? enabled : disabled);

    logDebug("%s Enable dynamic payload length data pipe 2 [DPL_P2]: ", regId);
    printf("%s", _CHECK_BIT(data, DYNPD_DPL_P2_2) ? enabled : disabled);

    logDebug("%s Enable dynamic payload length data pipe 1 [DPL_P1]: ", regId);
    printf("%s", _CHECK_BIT(data, DYNPD_DPL_P1_1) ? enabled : disabled);

    logDebug("%s Enable dynamic payload length data pipe 0 [DPL_P0]: ", regId);
    printf("%s", _CHECK_BIT(data, DYNPD_DPL_P0_0) ? enabled : disabled);
}

void NRF24L01p::printFeatureRegister()
{
    uint8_t data = readRegister(FEATURE);
    char regId[] = "[0x1D]";
    logDebug("%s Register [FEATURE] Feature Register: \r\n", regId);
    char enabled[] = "[1] Enabled\r\n";
    char disabled[] = "[0] Disabled\r\n";

    logDebug("%s Enables Dynamic Payload Length [EN_DPL]: ", regId);
    printf("%s", _CHECK_BIT(data, FEATURE_EN_DPL_2) ? enabled : disabled);

    logDebug("%s Enables Payload with ACK [EN_ACK_PAY]: ", regId);
    printf("%s", _CHECK_BIT(data, FEATURE_EN_ACK_PAY_1) ? enabled : disabled);

    logDebug("%s Enables the W_TX_PAYLOAD_NOACK command [EN_DYN_ACK]: ", regId);
    printf("%s", _CHECK_BIT(data, FEATURE_EN_DYN_ACK_0) ? enabled : disabled);
}