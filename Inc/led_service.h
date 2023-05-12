#ifndef __LED_SERVICE_H__
#define __LED_SERVICE_H__

#include "gpio.h"
#include "state.h"

class LedService {

private:
    uint32_t previousGreenHalTick = 0;
    uint16_t blinkGreenIntervalInTicks = 0;

public:
    LedService();
    void update(State* state);
    void blinkGreenLed(uint16_t intervalInTicks);
    void stopBlinkGreenLed();
};

#endif /* __LED_SERVICE_H__ */