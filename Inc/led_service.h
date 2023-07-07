#ifndef __LED_SERVICE_H__
#define __LED_SERVICE_H__

#include <stdio.h>

class LedService {

private:
    uint32_t greenLedPreviousTick = 0;
    uint16_t greenLedBlinkIntervalInTicks = 0;
    uint16_t greenLedNumberOfToggle = 0;
    uint16_t greenLedCurrentBlink = 0;
    uint16_t greenLedPouseIntervalInTicks = 0;
    bool greenLedStarted; 
    bool greenLedPoused;

public:
    LedService();
    void update();
    void blinkGreenLed(uint8_t numberOfBlinks = 0, uint16_t intervalInTicks = 200, uint16_t pouseIntervalInTicks = 2000);
    void stopBlinkGreenLed();
    void blinkGreenRedOrangeLedOneTime();
};

#endif /* __LED_SERVICE_H__ */