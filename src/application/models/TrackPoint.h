#ifndef TRACK_POINT_H
#define TRACK_POINT_H

#include <stdint.h>

struct TrackPoint
{
    float latitude;
    float longitude;
    float altitudeMeters;
    uint32_t timestampMilliseconds;
};

#endif
