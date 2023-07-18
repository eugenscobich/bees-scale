#ifndef __RADIO_SERVICE_H__
#define __RADIO_SERVICE_H__

#include "NRF24L01p.h"
#include "sensors_service.h"

class RadioService {

private:
    NRF24L01p* nRF24L01p;
    SensorsService* sensorsService;
    void(*updateFunction)();

    uint32_t startDelayTick;

    void printData(uint8_t *dataToPrint);

public:
    RadioService(NRF24L01p* nRF24L01p, SensorsService* _sensorsService, void(*updateFunction)());
    
    bool isRadioInRxMode();
    void readMasterData(uint8_t masterData[][32]);
    bool isDataReceived();
    void powerDown();
};

#endif /* __RADIO_SERVICE_H__ */