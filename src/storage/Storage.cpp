#include "Storage.h"
#include <Arduino.h>

bool Storage::begin()
{
    return true;
}

void Storage::update(const TrackPoint &point)
{
    if (!trackingEnabled)
    {
        return;
    }

    trackWriter.addPoint(point);
}

void Storage::setTrackingEnabled(bool enabled)
{
    if (enabled == trackingEnabled)
    {
        return;
    }

    trackingEnabled = enabled;

    if (trackingEnabled)
    {
        sessionIndex++;
        Serial.print("Storage session started: ");
        Serial.println(sessionIndex);
        return;
    }

    trackWriter.flush();
    Serial.print("Storage session ended: ");
    Serial.println(sessionIndex);
}

UserProfile Storage::loadUserProfile() const
{
    UserProfile profile = {};
    profile.weightKg = 75.0f;
    profile.ageYears = 30;
    profile.isMale = true;
    profile.restingHeartRateBpm = 60;
    return profile;
}
