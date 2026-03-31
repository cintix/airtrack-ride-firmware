#ifndef SCREEN_H
#define SCREEN_H

#include "Display.h"   // dit interface
#include <stdint.h>

class Screen {
private:
    Display& display;
    DisplayRecord lastRecord = {};
    uint32_t lastRefreshTimestampMilliseconds = 0;
    bool hasLastRecord = false;

    bool shouldRefresh(const DisplayRecord& record, uint32_t nowMilliseconds) const;
    bool hasSignificantChange(const DisplayRecord& previous, const DisplayRecord& current) const;

public:
    Screen(Display& display);
    void update(const DisplayRecord& record);
};

#endif
