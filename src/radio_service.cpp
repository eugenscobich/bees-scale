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
    return nRF24L01p->isPowerUp() && nRF24L01p->isInRxMode();
}

bool RadioService::isDataReceived() {
    return nRF24L01p->isDataReceived();
}

void RadioService::powerDown() {
    logInfo("Power down the radio\r\n");
    nRF24L01p->powerDown();
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