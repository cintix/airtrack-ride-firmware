#ifndef GPS_FIX_H
#define GPS_FIX_H

#include <stdint.h>

struct GpsFix
{
    double latitude;
    double longitude;

    float altitudeMeters;
    float groundSpeedMetersPerSecond;
    float headingDegrees;

    uint8_t satelliteCount;
    uint32_t timestampMilliseconds;
    bool hasUtcTime;
    uint32_t utcEpochSeconds;
};

#endif
