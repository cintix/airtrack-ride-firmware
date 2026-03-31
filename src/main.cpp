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

    GpsFix toGpsFix(const GpsRecord &record)
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
        return gpsFix;
    }

    DisplayRecord toDisplayRecord(const ApplicationResult &applicationResult)
    {
        DisplayRecord displayRecord = {};
        displayRecord.SpeedKm = applicationResult.displayData.speedKm;
        displayRecord.timeSpent = applicationResult.displayData.timeSpentSeconds;
        displayRecord.distanceKm = applicationResult.displayData.distanceKm;
        displayRecord.tempatureC = applicationResult.displayData.temperatureC;
        displayRecord.timeStarted = applicationResult.displayData.timeStartedSeconds;
        return displayRecord;
    }

    void printGpsDebug(const GpsRecord &record,
                       bool hasFix,
                       const ApplicationResult &applicationResult,
                       uint32_t &lastPrint,
                       int16_t userTimezoneOffsetMinutes)
    {
        if (millis() - lastPrint <= DEBUG_GPS_PRINT_INTERVAL_MS)
        {
            return;
        }

        lastPrint = millis();

        Serial.print("FIX: ");
        Serial.print(hasFix ? "YES" : "NO");
        Serial.print("  SAT: ");
        Serial.print(record.satelliteCount);
        Serial.print("  LAT: ");
        Serial.print(record.latitude, 7);
        Serial.print("  LON: ");
        Serial.print(record.longitude, 7);
        Serial.print("  SPEED: ");

        if (hasFix)
        {
            Serial.print(applicationResult.displayData.speedKm, 2);
        }
        else
        {
            Serial.print(record.groundSpeedMetersPerSecond * 3.6f, 2);
        }

        Serial.print(" km/h  DIST: ");
        Serial.print(applicationResult.stats.distanceMeters / 1000.0f, 2);
        Serial.print(" km  AVG: ");
        Serial.print(applicationResult.stats.averageSpeedKmh, 2);
        Serial.print(" km/h");

        if (record.hasUtcTime)
        {
            char utcBuffer[24];
            char localBuffer[24];

            uint32_t localEpochSeconds = applyTimezoneOffset(record.utcEpochSeconds, userTimezoneOffsetMinutes);
            formatDateTime(utcBuffer, sizeof(utcBuffer), record.utcEpochSeconds);
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
    applicationUserProfile.stoppedSpeedThresholdKmh = storedUserProfile.stoppedSpeedThresholdKmh;
    applicationUserProfile.stoppedDelaySeconds = storedUserProfile.stoppedDelaySeconds;
    userTimezoneOffsetMinutes = storedUserProfile.timezoneOffsetMinutes;

    application.begin(applicationUserProfile);

    input.begin();
    oled.begin();
    client.begin();
    client.setWifiEnabled(!application.isTrackingEnabled());

    // Ensure the display is visibly active even before the first valid GPS fix.
    screen.update({});

    Serial.println("AirTrack Ride firmware is ready...");
}

void loop()
{
    gps.update();
    input.update();

    static uint32_t lastPrint = 0;
    static ApplicationResult lastApplicationResult = {};
    static DisplayRecord lastDisplayRecord = {};

    if (input.IsToggled())
    {
        bool trackingEnabled = !application.isTrackingEnabled();
        application.setTrackingEnabled(trackingEnabled);
        storage.setTrackingEnabled(trackingEnabled);
        client.setWifiEnabled(!trackingEnabled);

        Serial.print("Tracking: ");
        Serial.println(trackingEnabled ? "ON" : "OFF");
    }

    bool hasRecord = gps.hasRecord();
    GpsRecord record = {};
    bool hasFix = false;

    if (hasRecord)
    {
        record = gps.getRecord();
        hasFix = record.valid;

        if (hasFix)
        {
            GpsFix gpsFix = toGpsFix(record);
            ApplicationResult applicationResult = application.update(gpsFix);
            lastApplicationResult = applicationResult;
            lastDisplayRecord = toDisplayRecord(applicationResult);

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
        }

        // Keep display alive regardless of GPS fix state.
        screen.update(lastDisplayRecord);
        printGpsDebug(record, hasFix, lastApplicationResult, lastPrint, userTimezoneOffsetMinutes);
    }

    client.update();
}
