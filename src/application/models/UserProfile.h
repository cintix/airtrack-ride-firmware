#ifndef USER_PROFILE_H
#define USER_PROFILE_H

#include <stdint.h>

struct UserProfile
{
    float weightKg;
    uint8_t ageYears;
    bool isMale;
    uint8_t restingHeartRateBpm;
    int16_t timezoneOffsetMinutes;
    float stoppedSpeedThresholdKmh;
    int16_t stoppedDelaySeconds;
};

#endif
