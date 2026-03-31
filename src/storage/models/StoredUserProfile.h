#ifndef STORED_USER_PROFILE_H
#define STORED_USER_PROFILE_H

#include <stdint.h>

struct StoredUserProfile
{
    float weightKg;
    uint8_t ageYears;
    bool isMale;
    uint8_t restingHeartRateBpm;
};

#endif
