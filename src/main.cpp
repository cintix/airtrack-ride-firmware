#include "Arduino.h"
#include "gps/GpsReader.h"
#include "application/Application.h"
#include "storage/Storage.h"
#include "client/ClientSync.h"
#include "config/Config.h"

GpsReader gps;
Application application;
Storage storage;
ClientSync client;

void setup()
{
    Serial.begin(115200);

    while (!Serial)
    {
        delay(10);
    }

    Serial.println();
    Serial.println("AirTrack Ride firmware starting...");

    gps.begin();

    if (!storage.begin())
    {
        Serial.println("Storage initialization failed");
    }
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

            Serial.print("Speed: ");
            Serial.print(speedKmh, 2);
            Serial.println(" km/h");
        }
    }
}