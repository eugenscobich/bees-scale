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
    void initRadio();
    bool isRadioInRxMode();
    void readMasterData(uint8_t masterData[][32]);
    bool isDataReceived();
    void powerDown();
    void stopListening();
    void openWritingPipe(uint64_t address);
    void openReadingPipe(uint64_t address, uint8_t pipeNumber);
    bool write(uint8_t *data);
    bool isDataAvailable();
    void receive(uint8_t *data);
    void writeAcknowledgePayload(uint8_t pipeNumber, uint8_t *data, uint8_t size);
    void flushTx();
    void startListening();
   
    
};

#endif /* __RADIO_SERVICE_H__ */