#ifndef PROFILE_SERVICE_H
#define PROFILE_SERVICE_H

#include <Arduino.h>

class ProfileService
{
public:
    String buildProfileJson() const;
    bool saveProfile(const String &weight,
                     const String &age,
                     const String &isMale,
                     const String &resting,
                     const String &timezone,
                     const String &stopThreshold,
                     const String &stopDelay) const;
};

#endif
