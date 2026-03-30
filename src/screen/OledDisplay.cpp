#include "OledDisplay.h"

OledDisplay::OledDisplay() : u8g(U8G2_R0) {}

void OledDisplay::begin() {
    u8g.begin();
}

void OledDisplay::updateDisplay(const DisplayRecord& record) {
    u8g.firstPage();
    do {
        u8g.setFont(u8g2_font_ncenB08_tr);
        u8g.drawStr(0, 20, "Speed:");

        char buffer[10];
        sprintf(buffer, "%.2f km/t", record.distanceKm);
        u8g.drawStr(0, 40, buffer);

    } while (u8g.nextPage());
}