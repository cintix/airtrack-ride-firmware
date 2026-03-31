#ifndef STORAGE_H
#define STORAGE_H

#include "../application/models/UserProfile.h"
#include "../application/models/TrackPoint.h"
#include "TrackWriter.h"

class Storage
{
public:
    bool begin();
    void update(const TrackPoint &point);
    void setTrackingEnabled(bool enabled);
    UserProfile loadUserProfile() const;

private:
    TrackWriter trackWriter;
    bool trackingEnabled = false;
    uint32_t sessionIndex = 0;
};

#endif
