#include "Input.h"
#include <Arduino.h>
#include "../config/Config.h"

void Input::begin()
{
    pinMode(INPUT_TOGGLE_PIN, INPUT_PULLUP);

    bool pressed = digitalRead(INPUT_TOGGLE_PIN) == LOW;
    lastReadingPressed = pressed;
    lastStablePressed = pressed;
    lastDebounceTimestampMilliseconds = millis();
}

void Input::update()
{
    bool pressed = digitalRead(INPUT_TOGGLE_PIN) == LOW;
    unsigned long now = millis();

    if (pressed != lastReadingPressed)
    {
        lastReadingPressed = pressed;
        lastDebounceTimestampMilliseconds = now;
    }

    if ((now - lastDebounceTimestampMilliseconds) < INPUT_DEBOUNCE_MS)
    {
        return;
    }

    if (pressed != lastStablePressed)
    {
        lastStablePressed = pressed;

        if (lastStablePressed)
        {
            toggled = true;
        }
    }
}

bool Input::IsToggled()
{
    bool hasToggled = toggled;
    toggled = false;
    return hasToggled;
}
