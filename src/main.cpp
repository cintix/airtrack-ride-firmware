#include "Arduino.h"
#include "gps/GpsReader.h"
#include "application/Application.h"
#include "storage/Storage.h"
#include "client/ClientSync.h"

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

    if (gps.hasRecord())
    {
        GpsRecord record = gps.getRecord();

        Serial.print("LAT: ");
        Serial.print(record.latitude, 7);

        Serial.print("  LON: ");
        Serial.print(record.longitude, 7);

        Serial.print("  SAT: ");
        Serial.print(record.satelliteCount);

        Serial.print("  SPEED(km/h): ");

        float speedKmh = record.groundSpeedMetersPerSecond * 3.6f;

        Serial.println(speedKmh, 2);
    }
}