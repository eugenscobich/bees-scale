#include "radio_service.h"
#include "rtc.h"
#include <stdio.h>
#include <cstring>

#define CLASS_NAME "RadioServ"
#include "log.h"

RadioService::RadioService(NRF24L01p* _nRF24L01p, SensorsService* _sensorsService, void(*_updateFunction)()) :
    nRF24L01p(_nRF24L01p),
    sensorsService(_sensorsService),
    updateFunction(_updateFunction)
{

}

bool RadioService::isRadioInRxMode() {
    logInfo("Is radio in RX mode\r\n");
    return nRF24L01p->isPowerUp() && nRF24L01p->isInRxMode();
}

bool RadioService::isDataReceived() {
    logInfo("Is data received\r\n");
    return nRF24L01p->isDataReceived();
}

void RadioService::powerDown() {
    logInfo("Power down the radio\r\n");
    nRF24L01p->powerDown();
}

void RadioService::initRadio() {
    logInfo("Init Radio\r\n");
    nRF24L01p->init();
    nRF24L01p->enablePayloadWithAknoladge();
    nRF24L01p->enableDynamicPayload(0);
    nRF24L01p->enableDynamicPayload(1);
}

void RadioService::stopListening() {
    logInfo("Stop Listining\r\n");
    nRF24L01p->stopListening();
}

void RadioService::openWritingPipe(uint64_t address) {
    logInfo("Open Writing Pipe\r\n");
    nRF24L01p->openWritingPipe(address);
}

bool RadioService::write(uint8_t *data) {
    logInfo("Write data ");
    printData(data);
    return nRF24L01p->write(data);
}

bool RadioService::isDataAvailable() {
    logInfo("Is data available\r\n");
    return nRF24L01p->isDataAvailable();
}

void RadioService::receive(uint8_t *data) {
    logInfo("Receive\r\n");
    nRF24L01p->receive(data);
    logInfo("Received ");
    printData(data);
}

void RadioService::openReadingPipe(uint64_t address, uint8_t pipeNumber) {
    logInfo("Open reading pipe: %llu in pine nr: %u\r\n", address, pipeNumber);
    nRF24L01p->openReadingPipe(address, pipeNumber);
}

void RadioService::writeAcknowledgePayload(uint8_t pipeNumber, uint8_t *data, uint8_t size) {
    logInfo("Write acknowledge payload ");
    printData(data);
    nRF24L01p->writeAcknowledgePayload(pipeNumber, data, size);
}

void RadioService::flushTx() {
    logInfo("Flush TX\r\n");
    nRF24L01p->flushTx();
}

void RadioService::startListening() {
    logInfo("Start Listening\r\n");
    nRF24L01p->startListening();
}

void RadioService::readMasterData(uint8_t masterData[][32]) {
    uint8_t i = 0;
    uint32_t startDelayTick = HAL_GetTick();
    memset(masterData[0], 0, 32);
    memset(masterData[1], 0, 32);
    memset(masterData[2], 0, 32);
    while (true)
    {
        if (nRF24L01p->isDataReceived())
        {
            nRF24L01p->receive(masterData[i]);
            logInfo("Received ");
            printData(masterData[i]);
            i++;
            if (i == 3) {
                break; // waits up to 3 requests
            }
        }
        else if (startDelayTick + 200 < HAL_GetTick())
        {
            logError("Timeout Receive data from master\r\n");
            break;
        }
    }
}

void RadioService::printData(uint8_t *dataToPrint) {
    printf("Data: 0x");
    for (uint8_t i = 0; i < 32; i++)
    {
        printf("%02X", dataToPrint[i]);
    }
    printf("\r\n");
}