#include "Screen.h"

Screen::Screen(Display& display) : display(display) {
}

void Screen::update(const DisplayRecord& record) {
    display.updateDisplay(record);
}