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

    GpsRecord currentRecord;
    bool recordAvailable = false;

    void handlePacket(const UbxPacket &packet);

    void decodePosition(const UbxPacket &packet);
    void decodeVelocity(const UbxPacket &packet);
    void decodeNavigation(const UbxPacket &packet);

    void configurePlatformModel(uint8_t platformModel);
};

#endif