#include "GpsReader.h"

#include <Arduino.h>

#include "../config/Config.h"

void GpsReader::begin()
{
    ubxReader.begin();

    delay(500);

#if GPS_ENABLE_DYNAMIC_PLATFORM_MODEL
    configurePlatformModel(GPS_PLATFORM_MODEL);
#endif
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
        decodeNavigationPositionVelocityTime(packet);
    }
}

void GpsReader::decodeNavigationPositionVelocityTime(const UbxPacket &packet)
{
    const uint8_t *payload = packet.payload;

    int32_t longitudeRaw;
    int32_t latitudeRaw;
    int32_t altitudeMillimeters;
    int32_t groundSpeedMillimetersPerSecond;
    int32_t headingScaled;

    memcpy(&longitudeRaw, payload + 24, sizeof(int32_t));
    memcpy(&latitudeRaw, payload + 28, sizeof(int32_t));

    memcpy(&altitudeMillimeters, payload + 32, sizeof(int32_t));

    memcpy(&groundSpeedMillimetersPerSecond, payload + 60, sizeof(int32_t));

    memcpy(&headingScaled, payload + 64, sizeof(int32_t));

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

void GpsReader::configurePlatformModel(uint8_t platformModel)
{
    uint8_t ubxMessage[] =
        {
            0xB5, 0x62, // UBX header

            0x06, 0x24, // CFG-NAV5

            0x24, 0x00, // payload length (36 bytes)

            0x01, 0x00, // mask: apply dynamic model

            platformModel, // dynamic platform model

            0x03, // fix mode (auto)

            0x00, 0x00, 0x00, 0x00, // fixed altitude
            0x10, 0x27, 0x00, 0x00, // fixed altitude variance

            0x05, // minimum elevation

            0x00, // reserved

            0xFA, 0x00, // position DOP mask

            0xFA, 0x00, // time DOP mask

            0x64, 0x00, // position accuracy mask

            0x2C, 0x01, // time accuracy mask

            0x00, // static hold threshold

            0x3C, // dynamic hold threshold

            0x00, // reserved

            0x00, 0x00, // reserved

            0x00, 0x00, // reserved

            0x00, 0x00, // reserved

            0x00, 0x00 // reserved
        };

    uint8_t checksumByteA = 0;
    uint8_t checksumByteB = 0;

    for (uint8_t index = 2; index < sizeof(ubxMessage); index++)
    {
        checksumByteA += ubxMessage[index];
        checksumByteB += checksumByteA;
    }

    Serial1.write(ubxMessage, sizeof(ubxMessage));
    Serial1.write(checksumByteA);
    Serial1.write(checksumByteB);
}