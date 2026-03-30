#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "Display.h"
#include <U8g2lib.h>

class OledDisplay : public Display {
private:
    U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g;

public:
    OledDisplay();

    void begin();
    void updateDisplay(const DisplayRecord& record) override;
};

#endif