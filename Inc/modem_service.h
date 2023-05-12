#ifndef __MODEM_SERVICE_H__
#define __MODEM_SERVICE_H__

#include "gpio.h"
#include "SIM800C.h"
#include <stdio.h>

typedef enum {
	MODEM_SUCCESS,
	MODEM_ERROR
} ModemServiceResultStatus;


class ModemService {

private:
    SIM800C* sim800c;
public:
    ModemService(SIM800C* _sim800c);
    bool isSIM800CPresent();
    ModemServiceResultStatus startSIM800CIfNeed();
    ModemServiceResultStatus findSMSWithSettingsAndConfigureModem();
};

#endif /* __MODEM_SERVICE_H__ */