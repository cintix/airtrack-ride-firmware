#include "GpsReader.h"
#include <Arduino.h>
#include <string.h>
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
    else if (packet.messageId == GpsProtocol::NavigationMessageId::TimeUtc)
    {
        decodeTimeUtc(packet);
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

void GpsReader::decodeTimeUtc(const UbxPacket &packet)
{
    if (packet.payloadLength < 20)
    {
        currentRecord.hasUtcTime = false;
        currentRecord.utcEpochSeconds = 0;
        return;
    }

    const uint8_t *payload = packet.payload;

    uint16_t year = 0;
    memcpy(&year, payload + 12, sizeof(uint16_t));

    uint8_t month = payload[14];
    uint8_t day = payload[15];
    uint8_t hour = payload[16];
    uint8_t minute = payload[17];
    uint8_t second = payload[18];
    uint8_t validFlags = payload[19];

    const uint8_t validUtcBit = 0x04;
    bool hasValidUtc = (validFlags & validUtcBit) != 0;
    bool hasValidDate = (year >= 1970 && month >= 1 && month <= 12 && day >= 1 && day <= 31);
    bool hasValidTime = (hour <= 23 && minute <= 59 && second <= 60);

    if (!hasValidUtc || !hasValidDate || !hasValidTime)
    {
        currentRecord.hasUtcTime = false;
        currentRecord.utcEpochSeconds = 0;
        return;
    }

    currentRecord.hasUtcTime = true;
    currentRecord.utcEpochSeconds = convertUtcToEpochSeconds(year, month, day, hour, minute, second);
}

bool GpsReader::isLeapYear(uint16_t year) const
{
    if (year % 400 == 0)
    {
        return true;
    }

    if (year % 100 == 0)
    {
        return false;
    }

    return (year % 4) == 0;
}

uint32_t GpsReader::convertUtcToEpochSeconds(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) const
{
    static const uint8_t daysInMonth[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    uint32_t daysSinceEpoch = 0;

    for (uint16_t currentYear = 1970; currentYear < year; currentYear++)
    {
        daysSinceEpoch += isLeapYear(currentYear) ? 366 : 365;
    }

    for (uint8_t currentMonth = 1; currentMonth < month; currentMonth++)
    {
        daysSinceEpoch += daysInMonth[currentMonth - 1];

        if (currentMonth == 2 && isLeapYear(year))
        {
            daysSinceEpoch += 1;
        }
    }

    daysSinceEpoch += static_cast<uint32_t>(day - 1);

    uint32_t epochSeconds = daysSinceEpoch * 86400UL;
    epochSeconds += static_cast<uint32_t>(hour) * 3600UL;
    epochSeconds += static_cast<uint32_t>(minute) * 60UL;
    epochSeconds += static_cast<uint32_t>(second > 59 ? 59 : second);

    return epochSeconds;
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
