#ifndef APPLICATION_H
#define APPLICATION_H

#include "models/GpsFix.h"
#include "models/UserProfile.h"
#include "models/ApplicationResult.h"

class Application
{
public:
    void begin(const UserProfile &userProfile);
    void setTrackingEnabled(bool enabled);
    bool isTrackingEnabled() const;
    ApplicationResult update(const GpsFix &gpsFix);

private:
    UserProfile profile = {};
    RideStats stats = {};

    bool initialized = false;
    bool trackingEnabled = false;
    bool hasLastRecord = false;
    bool hasLastTrackPoint = false;

    GpsFix lastFix = {};
    TrackPoint lastTrackPoint = {};

    uint32_t rideStartTimestampMilliseconds = 0;
    uint32_t lastTrackPointTimestampMilliseconds = 0;
    uint32_t stationaryDurationMilliseconds = 0;
    float movingStopSpeedThresholdKmh = 0.0f;
    uint16_t movingStopDelaySeconds = 0;

    void resetTrackingState();
    void resolveMovementThresholds();

    float calculateDistanceMeters(float latitudeA, float longitudeA, float latitudeB, float longitudeB) const;
    float estimateCyclingMet(float speedKmh) const;
};

#endif
