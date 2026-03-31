#include "TrackWriter.h"
#include <Arduino.h>

void TrackWriter::addPoint(const StoredTrackPoint &point)
{
    if (bufferedCount >= maxBufferedPoints)
    {
        flush();
    }

    bufferedPoints[bufferedCount] = point;
    bufferedCount++;
}

void TrackWriter::flush()
{
    if (bufferedCount == 0)
    {
        return;
    }

    Serial.print("Storage flush points: ");
    Serial.println(bufferedCount);
    bufferedCount = 0;
}
