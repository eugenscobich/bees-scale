#ifndef __SENSORS_SERVICE_H__
#define __SENSORS_SERVICE_H__

#include "HX711.h"
#include "DS18B20.h"
#include "tim.h"

typedef struct {
    HX711 *hx711;
    DS18B20 *ds18b20;
    bool isPresent;
} Sensor;

class SensorsService {

private:
    Sensor sensors[3];

public:
    SensorsService(HX711 *hx711_1, HX711 *hx711_2, HX711 *hx711_3, DS18B20 *ds18b20_1, DS18B20 *ds18b20_2, DS18B20 *ds18b20_3);
    void readSensors();
    void readSensors(uint8_t i);

    Sensor* getSensors();
    void calibrateScales(bool buttonIsPressed);
    void resetBackUpRegisters();
};

#endif /* __SENSORS_SERVICE_H__ */