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

    uint32_t redLedPreviousTick = 0;
    uint16_t redLedBlinkIntervalInTicks = 0;
    uint16_t redLedNumberOfToggle = 0;
    uint16_t redLedCurrentBlink = 0;
    uint16_t redLedPouseIntervalInTicks = 0;
    bool redLedStarted;
    bool redLedPoused;

    uint16_t redAndOrangeRedLedNumberOfBlinks = 0;
    uint16_t redAndOrangeOrangeLedNumberOfBlinks = 0;
    bool redAndOrangeLedStarted;
    bool redAndOrangeLedPoused;
    

public:
    LedService();
    void update();
    void blinkGreenLed(uint8_t numberOfBlinks = 0, uint16_t intervalInTicks = 200, uint16_t pouseIntervalInTicks = 2000);
    void blinkRedLed(uint8_t numberOfBlinks = 0, uint16_t intervalInTicks = 200, uint16_t pouseIntervalInTicks = 2000);
    void blinkRedAndOrangeLed(uint8_t numberOfRedBlinks = 0, uint8_t numberOfOrangeBlinks = 0, uint16_t intervalInTicks = 200, uint16_t pouseIntervalInTicks = 2000);
    void stopBlinkGreenLed();
    void stopBlinkRedLed();
    void stopBlinkRedAndOrangeLed();
    
    void blinkGreenRedOrangeLedOneTime();
};

#endif /* __LED_SERVICE_H__ */