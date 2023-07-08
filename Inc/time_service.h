#ifndef __TIME_SERVICE_H__
#define __TIME_SERVICE_H__

#include <stdio.h>

typedef enum {
    HOURS = 0,
    MINUTES,
    SECONDS
} TimeUnit;

class TimeService {

private:

public:
    TimeService();

    void setAlarmFor(uint8_t delta, TimeUnit timeUints);

    void populateDataWithDateAndTime(uint8_t *data);
    void populateDataWithAlarmTimeFor(uint8_t *data, uint8_t delta, TimeUnit timeUints);
    void setDateAndTime(uint8_t *data);
    void setAlarmTime(uint8_t *data);
};

#endif /* __TIME_SERVICE_H__ */