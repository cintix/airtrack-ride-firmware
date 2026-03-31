#include "Screen.h"
#include "../config/Config.h"
#include <Arduino.h>
#include <math.h>

Screen::Screen(Display& display) : display(display) {
}

void Screen::update(const DisplayRecord& record) {
    uint32_t nowMilliseconds = millis();
    if (!shouldRefresh(record, nowMilliseconds))
    {
        return;
    }

    display.updateDisplay(record);
    lastRecord = record;
    lastRefreshTimestampMilliseconds = nowMilliseconds;
    hasLastRecord = true;
}

bool Screen::shouldRefresh(const DisplayRecord& record, uint32_t nowMilliseconds) const
{
    if (!hasLastRecord)
    {
        return true;
    }

    uint32_t elapsedMilliseconds = nowMilliseconds - lastRefreshTimestampMilliseconds;
    bool significantChange = hasSignificantChange(lastRecord, record);

    if (elapsedMilliseconds >= SCREEN_MAX_REFRESH_INTERVAL_MS)
    {
        return true;
    }

    if (!significantChange)
    {
        return false;
    }

    return elapsedMilliseconds >= SCREEN_MIN_REFRESH_INTERVAL_MS;
}

bool Screen::hasSignificantChange(const DisplayRecord& previous, const DisplayRecord& current) const
{
    if (fabsf(current.SpeedKm - previous.SpeedKm) >= SCREEN_SPEED_DELTA_KMH)
    {
        return true;
    }

    if (fabsf(current.distanceKm - previous.distanceKm) >= SCREEN_DISTANCE_DELTA_KM)
    {
        return true;
    }

    if (fabsf(current.timeSpent - previous.timeSpent) >= SCREEN_TIME_DELTA_SECONDS)
    {
        return true;
    }

    return false;
}
