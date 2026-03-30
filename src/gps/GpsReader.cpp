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
    
    if (packet.messageClass != GpsProtocol::MessageClass::Navigation)
        return;

    if (packet.messageId == GpsProtocol::NavigationMessageId::PositionLatitudeLongitudeHeight)
    {
        decodePosition(packet);
    }
    else if (packet.messageId == GpsProtocol::NavigationMessageId::VelocityNorthEastDown)
    {
        decodeVelocity(packet);
    } 
    else if (packet.messageId == GpsProtocol::NavigationMessageId::NavigationSolution)
    {
        decodeNavigation(packet);
    }   

}

void GpsReader::decodePosition(const UbxPacket &packet)
{
    const uint8_t *payload = packet.payload;

    int32_t lon;
    int32_t lat;
    int32_t height;

    memcpy(&lon, payload + 4, sizeof(int32_t));
    memcpy(&lat, payload + 8, sizeof(int32_t));
    memcpy(&height, payload + 12, sizeof(int32_t));

    currentRecord.longitude = lon / 10000000.0;
    currentRecord.latitude = lat / 10000000.0;
    currentRecord.altitudeMeters = height / 1000.0f;

    currentRecord.timestampMilliseconds = millis();
}

void GpsReader::decodeVelocity(const UbxPacket &packet)
{
    const uint8_t *payload = packet.payload;

    int32_t speed3D;
    int32_t groundSpeed;
    int32_t heading;

    memcpy(&speed3D, payload + 16, sizeof(int32_t));
    memcpy(&groundSpeed, payload + 20, sizeof(int32_t));
    memcpy(&heading, payload + 24, sizeof(int32_t));

    currentRecord.groundSpeedMetersPerSecond = groundSpeed / 100.0f;
    currentRecord.headingDegrees = heading / 100000.0f;

    currentRecord.timestampMilliseconds = millis();

    recordAvailable = true;
}

void GpsReader::decodeNavigation(const UbxPacket &packet)
{
    const uint8_t *payload = packet.payload;

    uint8_t fixType = payload[10];
    uint8_t satellites = payload[47];

    currentRecord.valid = (fixType >= 3);
    currentRecord.satelliteCount = satellites;

    currentRecord.timestampMilliseconds = millis();
    recordAvailable = true;
}

void GpsReader::configurePlatformModel(uint8_t platformModel)
{
    uint8_t ubxMessage[] =
        {
            0xB5, 0x62,
            0x06, 0x24,
            0x24, 0x00,
            0x01, 0x00,
            platformModel,
            0x03,
            0x00, 0x00, 0x00, 0x00,
            0x10, 0x27, 0x00, 0x00,
            0x05,
            0x00,
            0xFA, 0x00,
            0xFA, 0x00,
            0x64, 0x00,
            0x2C, 0x01,
            0x00,
            0x3C,
            0x00,
            0x00, 0x00,
            0x00, 0x00,
            0x00, 0x00,
            0x00, 0x00
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