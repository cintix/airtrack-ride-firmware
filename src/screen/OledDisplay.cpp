#include "OledDisplay.h"
#include <stdio.h>

OledDisplay::OledDisplay() : u8g(U8G2_R0) {}

void OledDisplay::begin() {
    u8g.begin();
}

void OledDisplay::updateDisplay(const DisplayRecord& record) {
    u8g.firstPage();
    do {
        u8g.setFont(u8g2_font_6x12_tr);

        char speedBuffer[16];
        snprintf(speedBuffer, sizeof(speedBuffer), "Speed: %.1f", record.SpeedKm);
        u8g.drawStr(0, 20, speedBuffer);

        char distanceBuffer[20];
        snprintf(distanceBuffer, sizeof(distanceBuffer), "Dist : %.2f km", record.distanceKm);
        u8g.drawStr(0, 40, distanceBuffer);

        char temperatureBuffer[20];
        snprintf(temperatureBuffer, sizeof(temperatureBuffer), "Temp : %.1f C", record.tempatureC);
        u8g.drawStr(0, 60, temperatureBuffer);

    } while (u8g.nextPage());
}
