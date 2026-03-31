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

GpsReader gps;
Application application;
Storage storage;
ClientSync client;
Input input;

OledDisplay oled;
Screen screen(oled);

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
                Serial.println(" km/h");
            }
        }
    }

    client.update();
}
