#ifndef EPAPER_DISPLAY_H
#define EPAPER_DISPLAY_H

#include "Display.h"

class EPaperDisplay : public Display
{
public:
    void begin();
    void updateDisplay(const DisplayRecord &record) override;
};

#endif
