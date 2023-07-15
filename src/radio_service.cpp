#include "radio_service.h"
#include "rtc.h"
#include <stdio.h>
#include <cstring>

RadioService::RadioService(NRF24L01p* _nRF24L01p, SensorsService* _sensorsService, void(*_updateFunction)()) :
    nRF24L01p(_nRF24L01p),
    sensorsService(_sensorsService),
    updateFunction(_updateFunction)
{

}

bool RadioService::isRadioInRxMode() {
    return nRF24L01p->isPowerUp() && nRF24L01p->isInRxMode();
}