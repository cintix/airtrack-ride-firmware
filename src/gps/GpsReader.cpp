#include "GpsReader.h"
#include <Arduino.h>

void GpsReader::begin()
{
    ubxReader.begin();
}

void GpsReader::update()
{
    ubxReader.update();

    if (ubxReader.hasPacket())
    {
        UbxPacket packet = ubxReader.getPacket();

        handlePacket(packet);
    }
}

bool GpsReader::hasRecord()
{
    return recordAvailable;
}

GpsRecord GpsReader::getRecord()
{
    recordAvailable = false;
    return currentRecord;
}

void GpsReader::handlePacket(const UbxPacket &packet)
{
    if (packet.messageClass == 0x01 && packet.messageId == 0x07)
    {
        decodeNavPvt(packet);
    }
}

void GpsReader::decodeNavPvt(const UbxPacket &packet)
{
    const uint8_t *payload = packet.payload;

    int32_t longitudeRaw = *(int32_t *)(payload + 24);
    int32_t latitudeRaw = *(int32_t *)(payload + 28);

    int32_t altitudeMillimeters = *(int32_t *)(payload + 32);

    int32_t groundSpeedMillimetersPerSecond = *(int32_t *)(payload + 60);

    int32_t headingScaled = *(int32_t *)(payload + 64);

    uint8_t fixType = payload[20];
    uint8_t satelliteCount = payload[23];

    currentRecord.valid = (fixType >= 3);

    currentRecord.latitude = latitudeRaw / 10000000.0;
    currentRecord.longitude = longitudeRaw / 10000000.0;

    currentRecord.altitudeMeters = altitudeMillimeters / 1000.0f;

    currentRecord.groundSpeedMetersPerSecond =
        groundSpeedMillimetersPerSecond / 1000.0f;

    currentRecord.headingDegrees =
        headingScaled / 100000.0f;

    currentRecord.satelliteCount = satelliteCount;

    currentRecord.timestampMilliseconds = millis();

    recordAvailable = true;
}