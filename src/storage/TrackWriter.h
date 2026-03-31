#ifndef TRACK_WRITER_H
#define TRACK_WRITER_H

#include <stdint.h>
#include "../config/Config.h"
#include "../application/models/TrackPoint.h"

class TrackWriter
{
public:
    void addPoint(const TrackPoint& point);
    void flush();

private:
    static const uint8_t maxBufferedPoints = TRACK_BUFFER_SIZE;
    TrackPoint bufferedPoints[maxBufferedPoints];
    uint8_t bufferedCount = 0;
};

#endif
