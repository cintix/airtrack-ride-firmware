#ifndef APPLICATION_H
#define APPLICATION_H

#include "models/UserProfile.h"
#include "models/ApplicationResult.h"
#include "../gps/models/GpsRecord.h"

class Application
{
public:
    void begin(const UserProfile &userProfile);
    void setTrackingEnabled(bool enabled);
    bool isTrackingEnabled() const;
    ApplicationResult update(const GpsRecord &record);

private:
    UserProfile profile = {};
    RideStats stats = {};

    bool initialized = false;
    bool trackingEnabled = false;
    bool hasLastRecord = false;
    bool hasLastTrackPoint = false;

    GpsRecord lastRecord = {};
    TrackPoint lastTrackPoint = {};

    uint32_t rideStartTimestampMilliseconds = 0;
    uint32_t lastTrackPointTimestampMilliseconds = 0;

    void resetTrackingState();

    float calculateDistanceMeters(float latitudeA, float longitudeA, float latitudeB, float longitudeB) const;
    float estimateCyclingMet(float speedKmh) const;
};

#endif
