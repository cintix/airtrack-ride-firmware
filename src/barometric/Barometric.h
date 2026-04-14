#ifndef BAROMETRIC_H
#define BAROMETRIC_H

#include <Arduino.h>

struct BarometricReading
{
    float temperatureC;
    float pressureHpa;
    float humidityPercent;
    float altitudeMeters;
};

class Barometric
{
public:
    void begin();
    void update();

    bool isAvailable() const;
    bool hasReading() const;
    BarometricReading getReading();

private:
    bool available = false;
    bool readingAvailable = false;
    uint32_t lastReadTimestampMilliseconds = 0;
    BarometricReading latestReading = {};

    void readSensor();
};

#endif
