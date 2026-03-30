#ifndef DISPLAY_H
#define DISPLAY_H

#include "models/DisplayRecord.h"

class Display {
public:
    virtual ~Display() {}  
    virtual void updateDisplay(const DisplayRecord& record) = 0;
};

#endif