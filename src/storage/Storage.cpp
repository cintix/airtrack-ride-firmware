#include "Storage.h"
#include <Arduino.h>
#include <LittleFS.h>

namespace
{
    constexpr const char *PROFILE_CONFIG_PATH = "/config/profile.txt";

    bool parseKeyValueLine(const String &line, String &key, String &value)
    {
        int separator = line.indexOf('=');
        if (separator <= 0)
        {
            return false;
        }

        key = line.substring(0, separator);
        value = line.substring(separator + 1);
        key.trim();
        value.trim();
        return key.length() > 0;
    }
}

bool Storage::begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("Storage: LittleFS mount failed");
        return false;
    }

    if (!LittleFS.exists("/config"))
    {
        LittleFS.mkdir("/config");
    }

    File profileFile = LittleFS.open(PROFILE_CONFIG_PATH, "a");
    if (profileFile)
    {
        profileFile.close();
    }

    return true;
}

void Storage::update(const StoredTrackPoint &point)
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

StoredUserProfile Storage::loadUserProfile() const
{
    StoredUserProfile profile = {};
    profile.weightKg = 75.0f;
    profile.ageYears = 30;
    profile.isMale = true;
    profile.restingHeartRateBpm = 60;
    profile.timezoneOffsetMinutes = 60;
    profile.stoppedSpeedThresholdKmh = 0.0f;
    profile.stoppedDelaySeconds = 0;

    File file = LittleFS.open(PROFILE_CONFIG_PATH, "r");
    if (!file || file.isDirectory())
    {
        return profile;
    }

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        String key;
        String value;
        if (!parseKeyValueLine(line, key, value))
        {
            continue;
        }

        if (key == "weightKg")
        {
            profile.weightKg = value.toFloat();
        }
        else if (key == "ageYears")
        {
            profile.ageYears = static_cast<uint8_t>(value.toInt());
        }
        else if (key == "isMale")
        {
            profile.isMale = (value.toInt() != 0);
        }
        else if (key == "restingHeartRateBpm")
        {
            profile.restingHeartRateBpm = static_cast<uint8_t>(value.toInt());
        }
        else if (key == "timezoneOffsetMinutes")
        {
            profile.timezoneOffsetMinutes = static_cast<int16_t>(value.toInt());
        }
        else if (key == "stoppedSpeedThresholdKmh")
        {
            profile.stoppedSpeedThresholdKmh = value.toFloat();
        }
        else if (key == "stoppedDelaySeconds")
        {
            profile.stoppedDelaySeconds = static_cast<int16_t>(value.toInt());
        }
    }

    file.close();
    return profile;
}
