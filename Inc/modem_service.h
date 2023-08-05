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

class ModemService {

private:
    SIM800C* sim800c;
    SensorsService* sensorsService;
    void(*updateFunction)();
    void(*errorFunction)(uint8_t);

    SIM800CCmdResult* sim800cResult;
    uint32_t startDelayTick;

    bool foundSettingsSMS;

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
    void nonBlockingDelay(uint32_t delayInTicks);
    void changeSim800CPwrPinToOuput();

    void openBearer();

public:
    ModemService(SIM800C* _sim800c, SensorsService* _sensorsService, void(*updateFunction)(), void(*errorFunction)(uint8_t));
    bool isModemPresent();
    void startModemIfNeed();
    void checkModemHealth();
    void configureModem();
    void findSMSWithSettingsAndConfigureModem();
    void deleteAllSMS();
    void waitForSettingsSMS();
    void configureDateAndTime();
    void sendData(uint8_t sensorData[][32]);
    void powerDown();

    uint16_t getRefreshIntervalInMinutes() ;
    uint8_t getBatteryLevel();

    void disablePowerOnPin();
    void enablePowerOnPin();

    bool isSettingsSMSFound();
};

#endif /* __MODEM_SERVICE_H__ */