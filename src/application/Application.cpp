#include "Application.h"
#include <math.h>
#include "../config/Config.h"

void Application::begin(const UserProfile &userProfile)
{
    profile = userProfile;
    initialized = true;
    trackingEnabled = false;
    resetTrackingState();
}

void Application::setTrackingEnabled(bool enabled)
{
    if (enabled == trackingEnabled)
    {
        return;
    }

    trackingEnabled = enabled;

    if (trackingEnabled)
    {
        resetTrackingState();
    }
}

bool Application::isTrackingEnabled() const
{
    return trackingEnabled;
}

ApplicationResult Application::update(const GpsRecord &record)
{
    ApplicationResult result = {};
    result.hasDisplayData = true;
    result.hasTrackPoint = false;
    result.isTrackingActive = trackingEnabled;
    result.displayData.speedKm = record.groundSpeedMetersPerSecond * 3.6f;
    result.displayData.timeSpentSeconds = stats.elapsedSeconds;
    result.displayData.distanceKm = stats.distanceMeters / 1000.0f;
    result.displayData.temperatureC = 0.0f;
    result.displayData.timeStartedSeconds = rideStartTimestampMilliseconds / 1000.0f;
    result.stats = stats;

    if (!initialized)
    {
        begin({75.0f, 30, true, 60});
    }

    if (!record.valid || !trackingEnabled)
    {
        return result;
    }

    if (!hasLastRecord)
    {
        rideStartTimestampMilliseconds = record.timestampMilliseconds;
        lastRecord = record;
        hasLastRecord = true;
        stats.sampleCount = 1;
    }
    else
    {
        float segmentDistanceMeters = calculateDistanceMeters(
            static_cast<float>(lastRecord.latitude),
            static_cast<float>(lastRecord.longitude),
            static_cast<float>(record.latitude),
            static_cast<float>(record.longitude));

        if (segmentDistanceMeters > 0.0f && segmentDistanceMeters < 1000.0f)
        {
            stats.distanceMeters += segmentDistanceMeters;
        }

        uint32_t deltaMilliseconds = record.timestampMilliseconds - lastRecord.timestampMilliseconds;
        if (deltaMilliseconds > 0 && deltaMilliseconds < 120000)
        {
            float deltaHours = deltaMilliseconds / 3600000.0f;
            if (record.groundSpeedMetersPerSecond > 0.5f)
            {
                stats.movingSeconds += deltaMilliseconds / 1000;
            }
            stats.caloriesBurned += estimateCyclingMet(record.groundSpeedMetersPerSecond * 3.6f) * profile.weightKg * deltaHours;
        }

        lastRecord = record;
        stats.sampleCount++;
    }

    uint32_t elapsedMilliseconds = record.timestampMilliseconds - rideStartTimestampMilliseconds;
    stats.elapsedSeconds = elapsedMilliseconds / 1000;

    if (stats.elapsedSeconds > 0)
    {
        float elapsedHours = stats.elapsedSeconds / 3600.0f;
        stats.averageSpeedKmh = (stats.distanceMeters / 1000.0f) / elapsedHours;
    }
    else
    {
        stats.averageSpeedKmh = 0.0f;
    }

    if (!hasLastTrackPoint)
    {
        result.hasTrackPoint = true;
    }
    else
    {
        float distanceFromLastTrackPoint = calculateDistanceMeters(
            lastTrackPoint.latitude,
            lastTrackPoint.longitude,
            static_cast<float>(record.latitude),
            static_cast<float>(record.longitude));
        uint32_t elapsedSinceLastTrackPointSeconds = (record.timestampMilliseconds - lastTrackPointTimestampMilliseconds) / 1000;

        if (distanceFromLastTrackPoint >= TRACK_MIN_DISTANCE_METERS || elapsedSinceLastTrackPointSeconds >= TRACK_MIN_INTERVAL_SECONDS)
        {
            result.hasTrackPoint = true;
        }
    }

    if (result.hasTrackPoint)
    {
        result.trackPoint.latitude = static_cast<float>(record.latitude);
        result.trackPoint.longitude = static_cast<float>(record.longitude);
        result.trackPoint.altitudeMeters = record.altitudeMeters;
        result.trackPoint.timestampMilliseconds = record.timestampMilliseconds;

        lastTrackPoint = result.trackPoint;
        lastTrackPointTimestampMilliseconds = record.timestampMilliseconds;
        hasLastTrackPoint = true;
    }

    result.stats = stats;
    result.displayData.speedKm = record.groundSpeedMetersPerSecond * 3.6f;
    result.displayData.timeSpentSeconds = stats.elapsedSeconds;
    result.displayData.distanceKm = stats.distanceMeters / 1000.0f;
    result.displayData.temperatureC = 0.0f;
    result.displayData.timeStartedSeconds = rideStartTimestampMilliseconds / 1000.0f;

    return result;
}

void Application::resetTrackingState()
{
    stats = {};
    hasLastRecord = false;
    hasLastTrackPoint = false;

    lastRecord = {};
    lastTrackPoint = {};
    rideStartTimestampMilliseconds = 0;
    lastTrackPointTimestampMilliseconds = 0;
}

float Application::calculateDistanceMeters(float latitudeA, float longitudeA, float latitudeB, float longitudeB) const
{
    const double pi = 3.14159265358979323846;
    const double earthRadiusMeters = 6371000.0;

    double latitudeRadiansA = latitudeA * (pi / 180.0);
    double latitudeRadiansB = latitudeB * (pi / 180.0);
    double deltaLatitudeRadians = (latitudeB - latitudeA) * (pi / 180.0);
    double deltaLongitudeRadians = (longitudeB - longitudeA) * (pi / 180.0);

    double sinLatitude = sin(deltaLatitudeRadians / 2.0);
    double sinLongitude = sin(deltaLongitudeRadians / 2.0);

    double haversine = sinLatitude * sinLatitude + cos(latitudeRadiansA) * cos(latitudeRadiansB) * sinLongitude * sinLongitude;
    double centralAngle = 2.0 * atan2(sqrt(haversine), sqrt(1.0 - haversine));

    return static_cast<float>(earthRadiusMeters * centralAngle);
}

float Application::estimateCyclingMet(float speedKmh) const
{
    if (speedKmh < 16.0f)
    {
        return 4.0f;
    }
    if (speedKmh < 19.0f)
    {
        return 6.8f;
    }
    if (speedKmh < 22.5f)
    {
        return 8.0f;
    }
    if (speedKmh < 25.7f)
    {
        return 10.0f;
    }
    if (speedKmh < 30.6f)
    {
        return 12.0f;
    }
    return 15.8f;
}
