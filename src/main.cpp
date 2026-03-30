#include "Arduino.h"
#include "gps/GpsReader.h"
#include "application/Application.h"
#include "storage/Storage.h"
#include "client/ClientSync.h"
#include "screen/Screen.h"
#include "screen/OledDisplay.h"
#include "screen/models/DisplayRecord.h"
#include "config/Config.h"

GpsReader gps;
Application application;
Storage storage;
ClientSync client;

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

    oled.begin();

    Serial.println("AirTrack Ride firmware is ready...");
}

void loop()
{
    gps.update();
    application.update();
    storage.update();
    client.update();

    static uint32_t lastPrint = 0;

    if (gps.hasRecord())
    {
        GpsRecord record = gps.getRecord();

        if (millis() - lastPrint > DEBUG_GPS_PRINT_INTERVAL_MS)
        {
            lastPrint = millis();

            float speedKmh = record.groundSpeedMetersPerSecond * 3.6f;

            DisplayRecord displayRecord;
            displayRecord.SpeedKm = speedKmh;

            Serial.print("FIX: ");
            Serial.print(record.valid ? "YES" : "NO");

            Serial.print("  SAT: ");
            Serial.print(record.satelliteCount);

            Serial.print("  LAT: ");
            Serial.print(record.latitude, 7);

            Serial.print("  LON: ");
            Serial.print(record.longitude, 7);

            Serial.print("  SPEED: ");
            Serial.print(speedKmh, 2);
            Serial.println(" km/h");

            screen.update(displayRecord);
        }
    }
}