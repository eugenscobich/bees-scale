#ifndef __SENSORS_SERVICE_H__
#define __SENSORS_SERVICE_H__

#include "HX711.h"
#include "DS18B20.h"
#include "tim.h"

typedef struct {
    uint8_t rom[8];
    float calibration;
    int32_t offset;
} Calibration;

typedef struct {
    HX711 *hx711;
    DS18B20 *ds18b20;
    uint8_t rom[8];
    float temperature;
    float weight;
} Sensor;

class SensorsService {

private:
    Sensor sensors[3];
    Calibration calibrations[10]; 
    Calibration* _findScaleCalibration(uint8_t *rom);

public:
    SensorsService(HX711 *hx711_1, HX711 *hx711_2, HX711 *hx711_3, DS18B20 *ds18b20_1, DS18B20 *ds18b20_2, DS18B20 *ds18b20_3);
    void readSensors();

    Sensor* getSensors();
};

#endif /* __SENSORS_SERVICE_H__ */