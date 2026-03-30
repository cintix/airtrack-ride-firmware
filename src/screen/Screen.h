#ifndef SCREEN_H
#define SCREEN_H

#include "Display.h"   // dit interface

class Screen {
private:
    Display& display;

public:
    Screen(Display& display);
    void update(const DisplayRecord& record);
};

#endif