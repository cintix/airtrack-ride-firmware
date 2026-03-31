#ifndef APPLICATION_RESULT_H
#define APPLICATION_RESULT_H

#include "TrackPoint.h"
#include "RideStats.h"
#include "ApplicationDisplayData.h"

struct ApplicationResult
{
    bool hasTrackPoint;
    bool hasDisplayData;
    bool isTrackingActive;
    TrackPoint trackPoint;
    RideStats stats;
    ApplicationDisplayData displayData;
};

#endif
