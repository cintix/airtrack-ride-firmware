#include "ProfileService.h"
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

    String readKeyValueFromFile(const char *path, const char *targetKey)
    {
        File file = LittleFS.open(path, "r");
        if (!file || file.isDirectory())
        {
            return "";
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

            if (key == targetKey)
            {
                file.close();
                return value;
            }
        }

        file.close();
        return "";
    }
}

String ProfileService::buildProfileJson() const
{
    String weight = readKeyValueFromFile(PROFILE_CONFIG_PATH, "weightKg");
    String age = readKeyValueFromFile(PROFILE_CONFIG_PATH, "ageYears");
    String isMale = readKeyValueFromFile(PROFILE_CONFIG_PATH, "isMale");
    String resting = readKeyValueFromFile(PROFILE_CONFIG_PATH, "restingHeartRateBpm");
    String timezone = readKeyValueFromFile(PROFILE_CONFIG_PATH, "timezoneOffsetMinutes");
    String stopThreshold = readKeyValueFromFile(PROFILE_CONFIG_PATH, "stoppedSpeedThresholdKmh");
    String stopDelay = readKeyValueFromFile(PROFILE_CONFIG_PATH, "stoppedDelaySeconds");

    if (weight.length() == 0)
    {
        weight = "75.0";
    }
    if (age.length() == 0)
    {
        age = "30";
    }
    if (isMale.length() == 0)
    {
        isMale = "1";
    }
    if (resting.length() == 0)
    {
        resting = "60";
    }
    if (timezone.length() == 0)
    {
        timezone = "60";
    }
    if (stopThreshold.length() == 0)
    {
        stopThreshold = "0.0";
    }
    if (stopDelay.length() == 0)
    {
        stopDelay = "0";
    }

    String json = "{";
    json += "\"weightKg\":" + weight;
    json += ",\"ageYears\":" + age;
    json += ",\"isMale\":" + isMale;
    json += ",\"restingHeartRateBpm\":" + resting;
    json += ",\"timezoneOffsetMinutes\":" + timezone;
    json += ",\"stoppedSpeedThresholdKmh\":" + stopThreshold;
    json += ",\"stoppedDelaySeconds\":" + stopDelay;
    json += "}";
    return json;
}

bool ProfileService::saveProfile(const String &weight,
                                 const String &age,
                                 const String &isMale,
                                 const String &resting,
                                 const String &timezone,
                                 const String &stopThreshold,
                                 const String &stopDelay) const
{
    File file = LittleFS.open(PROFILE_CONFIG_PATH, "w");
    if (!file || file.isDirectory())
    {
        return false;
    }

    file.printf("weightKg=%s\n", weight.length() ? weight.c_str() : "75.0");
    file.printf("ageYears=%s\n", age.length() ? age.c_str() : "30");
    file.printf("isMale=%s\n", isMale.length() ? isMale.c_str() : "1");
    file.printf("restingHeartRateBpm=%s\n", resting.length() ? resting.c_str() : "60");
    file.printf("timezoneOffsetMinutes=%s\n", timezone.length() ? timezone.c_str() : "60");
    file.printf("stoppedSpeedThresholdKmh=%s\n", stopThreshold.length() ? stopThreshold.c_str() : "0.0");
    file.printf("stoppedDelaySeconds=%s\n", stopDelay.length() ? stopDelay.c_str() : "0");
    file.close();
    return true;
}
