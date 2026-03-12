#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#include "models/GpsRecord.h"

class GpsParser
{
public:
    bool parse(const char *line, GpsRecord *record);

private:
    float parseCoordinate(const char *value, const char *direction);
};

#endif