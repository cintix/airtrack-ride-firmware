#ifndef INPUT_H
#define INPUT_H

class Input
{
public:
    void begin();
    void update();
    bool IsToggled();

private:
    bool toggled = false;
    bool lastStablePressed = false;
    bool lastReadingPressed = false;
    unsigned long lastDebounceTimestampMilliseconds = 0;
};

#endif
