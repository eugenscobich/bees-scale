#include "sensors_service.h"
#include <stdio.h>

#define NUMBER_OF_KNOWN_CALIBRATIONS 1

SensorsService::SensorsService(HX711 *hx711_1, HX711 *hx711_2, HX711 *hx711_3, DS18B20 *ds18b20_1, DS18B20 *ds18b20_2, DS18B20 *ds18b20_3)
{
    sensors[0].hx711 = hx711_1;
    sensors[0].ds18b20 = ds18b20_1;

    sensors[1].hx711 = hx711_2;
    sensors[1].ds18b20 = ds18b20_2;

    sensors[2].hx711 = hx711_3;
    sensors[2].ds18b20 = ds18b20_3;

    calibrations[0].rom[0] = 0x00;
    calibrations[0].rom[1] = 0x00;
    calibrations[0].rom[2] = 0x00;
    calibrations[0].rom[3] = 0x00;
    calibrations[0].rom[4] = 0x00;
    calibrations[0].rom[5] = 0x00;
    calibrations[0].rom[6] = 0x00;
    calibrations[0].rom[7] = 0x00;
    calibrations[0].calibration = 100.0;
}

void SensorsService::readSensors() {
    for (uint8_t i = 0; i < 3; i++)
    {
        printf("Start read sensors: %d\r\n", i);
        sensors[1].hx711->disable();
        bool isSensorPresent = sensors[i].ds18b20->start();
        if (isSensorPresent) {
            sensors[i].ds18b20->write(0x33);  // read ROM
            for (uint8_t j = 8; j > 0; j--)
            {
                sensors[i].rom[j - 1] = sensors[i].ds18b20->read();
            }
            sensors[i].ds18b20->write(0x44);
            printf("Sensor %d ROM: %X%X%X%X%X%X%X%X\r\n", i, sensors[i].rom[0], sensors[i].rom[1], sensors[i].rom[2], sensors[i].rom[3], sensors[i].rom[4], sensors[i].rom[5], sensors[i].rom[6], sensors[i].rom[7]);
            HAL_Delay(800);
            sensors[i].ds18b20->start();
            sensors[i].ds18b20->write(0xCC);
            sensors[i].ds18b20->write(0xBE);  // Read Scratch-pad
            uint8_t lsByte = sensors[i].ds18b20->read();
            sensors[i].temperature = ((sensors[i].ds18b20->read() << 8) | lsByte) * 0.0625;
            printf("Sensor %d temperature: %d.%02d\r\n", i, (uint16_t)(sensors[i].temperature), (uint16_t)(((uint16_t)(sensors[i].temperature * 100)) % 100));

            Calibration* calibration = _findScaleCalibration(sensors[i].rom);
            if (calibration != NULL) {
                sensors[i].hx711->setCoeficient(calibration->calibration);
                sensors[i].hx711->setOffset(calibration->offset);
                sensors[i].weight = sensors[i].hx711->getWeight(2);
            } else {
                printf("Sensor %d. Calibration not found. Scale raw value: %d\r\n", i, (int)sensors[i].hx711->getAverageValue(2));
            }
        } else {
            printf("Sensor %d temperature is not present\r\n", i);
        }   
    }    
}

Calibration* SensorsService::_findScaleCalibration(uint8_t *rom) {
    for (uint8_t i = 0; i < NUMBER_OF_KNOWN_CALIBRATIONS; i++)
    {        
        if (calibrations[i].rom[0] == rom[0] &&
            calibrations[i].rom[1] == rom[1] &&
            calibrations[i].rom[2] == rom[2] &&
            calibrations[i].rom[3] == rom[3] &&
            calibrations[i].rom[4] == rom[4] &&
            calibrations[i].rom[5] == rom[5] &&
            calibrations[i].rom[6] == rom[6] &&
            calibrations[i].rom[7] == rom[7]) {
                return calibrations + i;
        }
    }
    return NULL;
}