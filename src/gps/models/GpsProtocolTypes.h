#ifndef GPS_PROTOCOL_TYPES_H
#define GPS_PROTOCOL_TYPES_H

#include <stdint.h>

namespace GpsProtocol
{
    namespace MessageClass
    {
        static const uint8_t Navigation = 0x01;
        static const uint8_t Configuration = 0x06;
    }

    namespace NavigationMessageId
    {
        static const uint8_t PositionLatitudeLongitudeHeight = 0x02; // POSLLH
        static const uint8_t VelocityNorthEastDown = 0x12;           // VELNED
        static const uint8_t TimeUtc = 0x21; // NAV-TIMEUTC
        static const uint8_t SatelliteInfo = 0x35; // NAV-SAT
        static const uint8_t NavigationSolution = 0x06; // NAV-SOL ✔
    }

    namespace ConfigurationMessageId
    {
        static const uint8_t SetMessageRate = 0x01; // CFG-MSG
        static const uint8_t SetRate = 0x08;        // CFG-RATE
        static const uint8_t Reset = 0x04;          // CFG-RST
    }

    namespace NmeaMessageId
    {
        static const uint8_t RecommendedMinimum = 0x04; // RMC
        static const uint8_t GlobalPositioningFix = 0x00; // GGA
    }
}

#endif
