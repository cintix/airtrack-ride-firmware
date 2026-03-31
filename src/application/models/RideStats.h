#ifndef RIDE_STATS_H
#define RIDE_STATS_H

#include <stdint.h>

struct RideStats
{
    float distanceMeters;
    float averageSpeedKmh;
    float caloriesBurned;
    uint32_t elapsedSeconds;
    uint32_t movingSeconds;
    uint32_t sampleCount;
};

#endif
