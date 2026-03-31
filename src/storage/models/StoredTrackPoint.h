#ifndef STORED_TRACK_POINT_H
#define STORED_TRACK_POINT_H

#include <stdint.h>

struct StoredTrackPoint
{
    float latitude;
    float longitude;
    float altitudeMeters;
    uint32_t timestampMilliseconds;
};

#endif
