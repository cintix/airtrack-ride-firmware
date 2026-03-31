#ifndef TRACK_WRITER_H
#define TRACK_WRITER_H

#include <stdint.h>
#include "../config/Config.h"
#include "models/StoredTrackPoint.h"

class TrackWriter
{
public:
    void addPoint(const StoredTrackPoint &point);
    void flush();

private:
    static const uint8_t maxBufferedPoints = TRACK_BUFFER_SIZE;
    StoredTrackPoint bufferedPoints[maxBufferedPoints];
    uint8_t bufferedCount = 0;
};

#endif
