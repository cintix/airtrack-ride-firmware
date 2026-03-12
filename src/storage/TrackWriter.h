#ifndef TRACK_WRITER_H
#define TRACK_WRITER_H

#include "../application/models/TrackPoint.h"

class TrackWriter
{
public:
    void addPoint(const TrackPoint& point);
    void flush();
};

#endif
