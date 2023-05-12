#ifndef __MODEM_SERVICE_H__
#define __MODEM_SERVICE_H__

#include "gpio.h"
#include "SIM800C.h"
#include "state.h"
#include "modem_state.h"
#include <stdio.h>

class ModemService {

private:
    SIM800C* sim800c;
    ModemState modemState;
    uint8_t numberOfRetries;
public:
    ModemService(SIM800C* _sim800c);
    bool isModemPresent();
    void update(State* state);
    void checkModemAndPowerOnIfNeed();
};

#endif /* __MODEM_SERVICE_H__ */