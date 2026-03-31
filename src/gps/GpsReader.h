#ifndef GPS_READER_H
#define GPS_READER_H

#include <stdint.h>

#include "models/GpsRecord.h"
#include "models/UbxPacket.h"
#include "models/GpsProtocolTypes.h"
#include "UbxReader.h"

class GpsReader
{
public:
    void begin();
    void update();

    bool hasRecord();
    GpsRecord getRecord();

private:
    UbxReader ubxReader;

    GpsRecord currentRecord = {};
    bool recordAvailable = false;

    void handlePacket(const UbxPacket &packet);

    void decodePosition(const UbxPacket &packet);
    void decodeVelocity(const UbxPacket &packet);
    void decodeNavigation(const UbxPacket &packet);
    void decodeTimeUtc(const UbxPacket &packet);

    bool isLeapYear(uint16_t year) const;
    uint32_t convertUtcToEpochSeconds(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) const;

    void configurePlatformModel(uint8_t platformModel);
};

#endif
