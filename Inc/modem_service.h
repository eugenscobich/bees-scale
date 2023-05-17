#ifndef __MODEM_SERVICE_H__
#define __MODEM_SERVICE_H__

#include "gpio.h"
#include "SIM800C.h"
#include <stdio.h>

#define APN_MAX_LENGTH 20
#define USER_MAX_LENGTH 20
#define PWD_MAX_LENGTH 20
#define API_KEY_MAX_LENGTH 20
#define HOST_MAX_LENGTH 20

typedef enum {
	MODEM_SUCCESS,
	MODEM_ERROR,
    MODEM_ERROR_IT_DIDN_T_REPONSD_AFTER_POWER_ON,
    MODEM_ERROR_SETTINGS_SMS_WASN_T_FOUND,
    MODEM_ERROR_RECEIVED_SMS_DOESN_T_CONTAINS_SETTINGS,
    MODEM_ERROR_SMS_RECEIVED_TIMEOUT
} ModemServiceResultStatus;

class ModemService {

private:
    SIM800C* sim800c;
    void(*updateFunction)();

    SIM800CCmdResult* sim800cResult;
    uint32_t startDelayTick;

    uint8_t signalQuality = 0;
    uint8_t batteryLevel = 0;
    uint16_t batteryVoltage = 0;
    char apn[APN_MAX_LENGTH + 1];
    char user[USER_MAX_LENGTH + 1];
    char pwd[PWD_MAX_LENGTH + 1];
    char apiKey[API_KEY_MAX_LENGTH + 1];
    char host[HOST_MAX_LENGTH + 1];
    void _nonBlockingDelay(uint32_t delayInTicks);
public:
    ModemService(SIM800C* _sim800c, void(*updateFunction)());
    bool isSIM800CPresent();
    ModemServiceResultStatus startModemIfNeed();
    ModemServiceResultStatus checkModemHealth();
    ModemServiceResultStatus configureModem();
    ModemServiceResultStatus findSMSWithSettingsAndConfigureModem();
    ModemServiceResultStatus deleteAllSMS();
    ModemServiceResultStatus waitForSettingsSMS();
    ModemServiceResultStatus configureDateAndTime();

    void disablePowerOnPin();
    void enablePowerOnPin();
};

#endif /* __MODEM_SERVICE_H__ */