#ifndef CLIENT_API_H
#define CLIENT_API_H

#include <WebServer.h>
#include "WifiManager.h"
#include "ProfileService.h"

class ClientApi
{
public:
    ClientApi(WifiManager &wifiManager, ProfileService &profileService);
    void registerRoutes(WebServer &server);

private:
    WifiManager &wifi;
    ProfileService &profiles;
    WebServer *server = nullptr;

    void handleStatus();
    void handleWifiScan();
    void handleWifiSave();
    void handleProfileGet();
    void handleProfileSave();
    void handleRides();
    void handleReboot();
};

#endif
