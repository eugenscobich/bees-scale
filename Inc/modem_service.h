#ifndef __MODEM_SERVICE_H__
#define __MODEM_SERVICE_H__

#include "gpio.h"
#include "SIM800C.h"
#include <stdio.h>

typedef enum {
	MODEM_SUCCESS,
	MODEM_ERROR,
    MODEM_ERROR_IT_DIDN_T_REPONSD_AFTER_POWER_ON
} ModemServiceResultStatus;

class ModemService {

private:
    SIM800C* sim800c;
    SIM800CCmdResult* sim800cResult;

    uint8_t signalQuality = 0;
    uint8_t batteryLevel = 0;
    uint16_t batteryVoltage = 0;
    
public:
    ModemService(SIM800C* _sim800c);
    bool isSIM800CPresent();
    ModemServiceResultStatus startModemIfNeed();
    ModemServiceResultStatus checkModemHealth();
    ModemServiceResultStatus configureModem();
    ModemServiceResultStatus findSMSWithSettingsAndConfigureModem();
    ModemServiceResultStatus deleteAllSMS();

    void disablePowerOnPin();
};

#endif /* __MODEM_SERVICE_H__ */