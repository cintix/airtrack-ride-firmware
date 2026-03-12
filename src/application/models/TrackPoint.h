#ifndef TRACK_POINT_H
#define TRACK_POINT_H

#include <stdint.h>

struct TrackPoint
{
    float latitude;
    float longitude;
    float altitude;
    uint32_t timestamp;
};

#endif
