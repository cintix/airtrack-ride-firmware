#include "Arduino.h"
#include "gps/GpsReader.h"
#include "application/Application.h"
#include "storage/Storage.h"
#include "client/ClientSync.h"
#include "screen/Screen.h"
#include "screen/OledDisplay.h"
#include "screen/models/DisplayRecord.h"
#include "input/Input.h"
#include "config/Config.h"

namespace
{
    struct DateTimeParts
    {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    };

    bool isLeapYear(uint16_t year)
    {
        if ((year % 400) == 0)
        {
            return true;
        }

        if ((year % 100) == 0)
        {
            return false;
        }

        return (year % 4) == 0;
    }

    DateTimeParts convertEpochToDateTime(uint32_t epochSeconds)
    {
        static const uint8_t daysInMonth[] = {
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };

        DateTimeParts result = {};

        uint32_t days = epochSeconds / 86400UL;
        uint32_t secondsInDay = epochSeconds % 86400UL;

        result.hour = secondsInDay / 3600UL;
        result.minute = (secondsInDay % 3600UL) / 60UL;
        result.second = secondsInDay % 60UL;

        uint16_t year = 1970;
        while (true)
        {
            uint16_t daysInYear = isLeapYear(year) ? 366 : 365;
            if (days < daysInYear)
            {
                break;
            }

            days -= daysInYear;
            year++;
        }

        result.year = year;

        uint8_t month = 1;
        while (month <= 12)
        {
            uint8_t monthDays = daysInMonth[month - 1];
            if (month == 2 && isLeapYear(year))
            {
                monthDays++;
            }

            if (days < monthDays)
            {
                break;
            }

            days -= monthDays;
            month++;
        }

        result.month = month;
        result.day = static_cast<uint8_t>(days + 1);
        return result;
    }

    uint32_t applyTimezoneOffset(uint32_t utcEpochSeconds, int16_t timezoneOffsetMinutes)
    {
        int64_t localEpoch = static_cast<int64_t>(utcEpochSeconds) + (static_cast<int64_t>(timezoneOffsetMinutes) * 60LL);
        if (localEpoch < 0)
        {
            return 0;
        }

        return static_cast<uint32_t>(localEpoch);
    }

    void formatDateTime(char *buffer, size_t bufferSize, uint32_t epochSeconds)
    {
        DateTimeParts parts = convertEpochToDateTime(epochSeconds);
        snprintf(buffer, bufferSize, "%04u-%02u-%02u %02u:%02u:%02u",
                 parts.year, parts.month, parts.day, parts.hour, parts.minute, parts.second);
    }
}

GpsReader gps;
Application application;
Storage storage;
ClientSync client;
Input input;

OledDisplay oled;
Screen screen(oled);
int16_t userTimezoneOffsetMinutes = 0;

void setup()
{
    Serial.begin(115200);

    delay(200); // USB settle

    Serial.println();
    Serial.println("AirTrack Ride firmware starting...");

    gps.begin();

    if (!storage.begin())
    {
        Serial.println("Storage initialization failed");
    }

    StoredUserProfile storedUserProfile = storage.loadUserProfile();
    UserProfile applicationUserProfile = {};
    applicationUserProfile.weightKg = storedUserProfile.weightKg;
    applicationUserProfile.ageYears = storedUserProfile.ageYears;
    applicationUserProfile.isMale = storedUserProfile.isMale;
    applicationUserProfile.restingHeartRateBpm = storedUserProfile.restingHeartRateBpm;
    applicationUserProfile.timezoneOffsetMinutes = storedUserProfile.timezoneOffsetMinutes;
    userTimezoneOffsetMinutes = storedUserProfile.timezoneOffsetMinutes;

    application.begin(applicationUserProfile);

    input.begin();
    oled.begin();

    Serial.println("AirTrack Ride firmware is ready...");
}

void loop()
{
    gps.update();
    input.update();

    static uint32_t lastPrint = 0;

    if (input.IsToggled())
    {
        bool trackingEnabled = !application.isTrackingEnabled();
        application.setTrackingEnabled(trackingEnabled);
        storage.setTrackingEnabled(trackingEnabled);

        Serial.print("Tracking: ");
        Serial.println(trackingEnabled ? "ON" : "OFF");
    }

    if (gps.hasRecord())
    {
        GpsRecord record = gps.getRecord();

        if (record.valid)
        {
            GpsFix gpsFix = {};
            gpsFix.latitude = record.latitude;
            gpsFix.longitude = record.longitude;
            gpsFix.altitudeMeters = record.altitudeMeters;
            gpsFix.groundSpeedMetersPerSecond = record.groundSpeedMetersPerSecond;
            gpsFix.headingDegrees = record.headingDegrees;
            gpsFix.satelliteCount = record.satelliteCount;
            gpsFix.timestampMilliseconds = record.timestampMilliseconds;
            gpsFix.hasUtcTime = record.hasUtcTime;
            gpsFix.utcEpochSeconds = record.utcEpochSeconds;

            ApplicationResult applicationResult = application.update(gpsFix);

            DisplayRecord displayRecord;
            displayRecord.SpeedKm = applicationResult.displayData.speedKm;
            displayRecord.timeSpent = applicationResult.displayData.timeSpentSeconds;
            displayRecord.distanceKm = applicationResult.displayData.distanceKm;
            displayRecord.tempatureC = applicationResult.displayData.temperatureC;
            displayRecord.timeStarted = applicationResult.displayData.timeStartedSeconds;
            screen.update(displayRecord);

            if (applicationResult.hasTrackPoint)
            {
                StoredTrackPoint storedTrackPoint = {};
                storedTrackPoint.latitude = applicationResult.trackPoint.latitude;
                storedTrackPoint.longitude = applicationResult.trackPoint.longitude;
                storedTrackPoint.altitudeMeters = applicationResult.trackPoint.altitudeMeters;
                storedTrackPoint.timestampMilliseconds = applicationResult.trackPoint.timestampMilliseconds;
                storedTrackPoint.utcEpochSeconds = gpsFix.hasUtcTime ? gpsFix.utcEpochSeconds : 0;
                storage.update(storedTrackPoint);
            }

            if (millis() - lastPrint > DEBUG_GPS_PRINT_INTERVAL_MS)
            {
                lastPrint = millis();

                Serial.print("FIX: YES");
                Serial.print("  SAT: ");
                Serial.print(record.satelliteCount);
                Serial.print("  LAT: ");
                Serial.print(record.latitude, 7);
                Serial.print("  LON: ");
                Serial.print(record.longitude, 7);
                Serial.print("  SPEED: ");
                Serial.print(applicationResult.displayData.speedKm, 2);
                Serial.print(" km/h");
                Serial.print("  DIST: ");
                Serial.print(applicationResult.stats.distanceMeters / 1000.0f, 2);
                Serial.print(" km");
                Serial.print("  AVG: ");
                Serial.print(applicationResult.stats.averageSpeedKmh, 2);
                Serial.print(" km/h");

                if (gpsFix.hasUtcTime)
                {
                    char utcBuffer[24];
                    char localBuffer[24];

                    uint32_t localEpochSeconds = applyTimezoneOffset(gpsFix.utcEpochSeconds, userTimezoneOffsetMinutes);
                    formatDateTime(utcBuffer, sizeof(utcBuffer), gpsFix.utcEpochSeconds);
                    formatDateTime(localBuffer, sizeof(localBuffer), localEpochSeconds);

                    Serial.print("  UTC: ");
                    Serial.print(utcBuffer);
                    Serial.print("  LOCAL: ");
                    Serial.println(localBuffer);
                }
                else
                {
                    Serial.println("  UTC: N/A");
                }
            }
        }
    }

    client.update();
}
