#ifndef GPS_RESULT_H
#define GPS_RESULT_H

#include <stdint.h>

struct GpsResult
{
    bool hasFix;
    float latitude;
    float longitude;
    float speedKmh;
    uint32_t lastUpdate;
};

#endif
