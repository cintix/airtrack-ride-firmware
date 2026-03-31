#ifndef CLIENT_SYNC_H
#define CLIENT_SYNC_H

#include "WifiManager.h"
#include "ProfileService.h"
#include "ClientApi.h"
#include "HttpServerHost.h"

class ClientSync
{
public:
    ClientSync();

    void begin();
    void update();
    void setWifiEnabled(bool enabled);

private:
    bool initialized = false;
    bool wifiEnabled = true;
    WifiManager wifiManager;
    ProfileService profileService;
    ClientApi clientApi;
    HttpServerHost httpServer;
};

#endif
