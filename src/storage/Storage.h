#ifndef STORAGE_H
#define STORAGE_H

#include "models/StoredUserProfile.h"
#include "models/StoredTrackPoint.h"
#include "TrackWriter.h"

class Storage
{
public:
    bool begin();
    void update(const StoredTrackPoint &point);
    void setTrackingEnabled(bool enabled);
    StoredUserProfile loadUserProfile() const;

private:
    TrackWriter trackWriter;
    bool trackingEnabled = false;
    uint32_t sessionIndex = 0;
};

#endif
