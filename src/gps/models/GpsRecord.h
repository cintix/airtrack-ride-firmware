#ifndef GPS_RECORD_H
#define GPS_RECORD_H

#include <stdint.h>

struct GpsRecord
{
    bool valid;

    double latitude;
    double longitude;

    float altitudeMeters;

    float groundSpeedMetersPerSecond;
    float headingDegrees;

    uint8_t satelliteCount;

    uint32_t timestampMilliseconds;
};

#endif