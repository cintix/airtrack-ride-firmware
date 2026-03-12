#include "GpsParser.h"
#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

bool GpsParser::parse(const char *line, GpsRecord *record)
{
    if (strncmp(line, "$GPRMC", 6) != 0)
        return false;

    char buffer[120];
    strcpy(buffer, line);

    char *token;
    char *rest = buffer;

    int field = 0;

    char *lat = nullptr;
    char *latDir = nullptr;
    char *lon = nullptr;
    char *lonDir = nullptr;
    char *speed = nullptr;
    char *status = nullptr;

    while ((token = strtok_r(rest, ",", &rest)))
    {
        switch (field)
        {
        case 2:
            status = token;
            break;
        case 3:
            lat = token;
            break;
        case 4:
            latDir = token;
            break;
        case 5:
            lon = token;
            break;
        case 6:
            lonDir = token;
            break;
        case 7:
            speed = token;
            break;
        }

        field++;
    }

    if (!status || status[0] != 'A')
        return false;

    record->valid = true;
    record->latitude = parseCoordinate(lat, latDir);
    record->longitude = parseCoordinate(lon, lonDir);
    record->speedKnots = speed ? atof(speed) : 0;
    record->timestamp = millis();

    return true;
}

float GpsParser::parseCoordinate(const char *value, const char *direction)
{
    if (!value || !direction)
        return 0;

    float raw = atof(value);

    int degrees = (int)(raw / 100);
    float minutes = raw - (degrees * 100);

    float result = degrees + (minutes / 60.0f);

    if (direction[0] == 'S' || direction[0] == 'W')
        result = -result;

    return result;
}