#ifndef __MODEM_SERVICE_H__
#define __MODEM_SERVICE_H__

#include "SIM800C.h"
#include "sensors_service.h"

#define PHONE_NUMBER_MAX_LENGTH 20
#define APN_MAX_LENGTH 20
#define USER_MAX_LENGTH 20
#define PWD_MAX_LENGTH 20
#define API_KEY_MAX_LENGTH 30
#define HOST_MAX_LENGTH 20

typedef enum {
	MODEM_SUCCESS,
	MODEM_ERROR,
    MODEM_ERROR_IT_DIDN_T_REPONSD_AFTER_POWER_ON,
    MODEM_ERROR_SETTINGS_SMS_WASN_T_FOUND,
    MODEM_ERROR_RECEIVED_SMS_DOESN_T_CONTAINS_SETTINGS,
    MODEM_ERROR_SMS_RECEIVED_TIMEOUT,
    MODEM_ERROR_COULD_NOT_START_GPRS
} ModemServiceResultStatus;

class ModemService {

private:
    SIM800C* sim800c;
    SensorsService* sensorsService;
    void(*updateFunction)();

    SIM800CCmdResult* sim800cResult;
    uint32_t startDelayTick;

    uint8_t signalQuality = 0;
    uint8_t batteryLevel = 0;
    uint16_t batteryVoltage = 0;
    char phoneNumber[PHONE_NUMBER_MAX_LENGTH + 1];
    char apn[APN_MAX_LENGTH + 1];
    char user[USER_MAX_LENGTH + 1];
    char pwd[PWD_MAX_LENGTH + 1];
    char apiKey[API_KEY_MAX_LENGTH + 1];
    char host[HOST_MAX_LENGTH + 1];
    uint16_t refreshIntervalInMinutes;
    void _nonBlockingDelay(uint32_t delayInTicks);
    void _changeSim800CPwrPinToOuput();
public:
    ModemService(SIM800C* _sim800c, SensorsService* _sensorsService, void(*updateFunction)());
    bool isSIM800CPresent();
    ModemServiceResultStatus startModemIfNeed();
    ModemServiceResultStatus checkModemHealth();
    ModemServiceResultStatus configureModem();
    ModemServiceResultStatus findSMSWithSettingsAndConfigureModem();
    ModemServiceResultStatus deleteAllSMS();
    ModemServiceResultStatus waitForSettingsSMS();
    ModemServiceResultStatus configureDateAndTime();
    ModemServiceResultStatus sendData(uint8_t sensorData[][32]);
    ModemServiceResultStatus powerDown();

    uint16_t getRefreshIntervalInMinutes() ;
    uint8_t getBatteryLevel();

    void disablePowerOnPin();
    void enablePowerOnPin();
};

#endif /* __MODEM_SERVICE_H__ */