#include "Barometric.h"
#include "../config/Config.h"

#include <Adafruit_BME280.h>
#include <Wire.h>
#include <math.h>

namespace
{
    Adafruit_BME280 bmeSensor;

    bool isFinite(float value)
    {
        return !isnan(value) && !isinf(value);
    }
}

void Barometric::begin()
{
#if BAROMETRIC_ENABLED
    Wire.begin(BAROMETRIC_I2C_SDA_PIN, BAROMETRIC_I2C_SCL_PIN);

    available = bmeSensor.begin(BAROMETRIC_I2C_ADDRESS, &Wire);
    if (!available)
    {
        uint8_t fallbackAddress = (BAROMETRIC_I2C_ADDRESS == 0x76) ? 0x77 : 0x76;
        available = bmeSensor.begin(fallbackAddress, &Wire);
    }

    if (!available)
    {
        Serial.println("Barometric: BME280 not detected on I2C");
        return;
    }

    bmeSensor.setSampling(Adafruit_BME280::MODE_NORMAL,
                          Adafruit_BME280::SAMPLING_X2,
                          Adafruit_BME280::SAMPLING_X16,
                          Adafruit_BME280::SAMPLING_X1,
                          Adafruit_BME280::FILTER_X16,
                          Adafruit_BME280::STANDBY_MS_500);

    Serial.println("Barometric: BME280 initialized");
#else
    Serial.println("Barometric: disabled by config");
#endif
}

void Barometric::update()
{
#if BAROMETRIC_ENABLED
    if (!available)
    {
        return;
    }

    uint32_t nowMilliseconds = millis();
    if ((nowMilliseconds - lastReadTimestampMilliseconds) < BAROMETRIC_READ_INTERVAL_MS)
    {
        return;
    }

    lastReadTimestampMilliseconds = nowMilliseconds;
    readSensor();
#endif
}

bool Barometric::isAvailable() const
{
    return available;
}

bool Barometric::hasReading() const
{
    return readingAvailable;
}

BarometricReading Barometric::getReading() const
{
    return latestReading;
}

void Barometric::readSensor()
{
    float temperatureC = bmeSensor.readTemperature();
    float pressureHpa = bmeSensor.readPressure() / 100.0f;
    float humidityPercent = bmeSensor.readHumidity();
    float altitudeMeters = bmeSensor.readAltitude(BAROMETRIC_SEA_LEVEL_PRESSURE_HPA);

    if (!isFinite(temperatureC) || !isFinite(pressureHpa) || !isFinite(humidityPercent) || !isFinite(altitudeMeters))
    {
        return;
    }

    latestReading.temperatureC = temperatureC;
    latestReading.pressureHpa = pressureHpa;
    latestReading.humidityPercent = humidityPercent;
    latestReading.altitudeMeters = altitudeMeters;
    readingAvailable = true;
}
