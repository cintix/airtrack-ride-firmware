#ifndef GPS_READER_H
#define GPS_READER_H

#include "models/GpsRecord.h"
#include "models/UbxPacket.h"
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

    void decodeNavPvt(const UbxPacket &packet);
};

#endif