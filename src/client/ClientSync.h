#ifndef CLIENT_SYNC_H
#define CLIENT_SYNC_H

#include <WebServer.h>

class ClientSync
{
public:
    void begin();
    void update();

private:
    enum class RequestContext
    {
        AccessPoint,
        Station
    };

    struct WifiCredentials
    {
        String ssid;
        String password;
    };

    bool initialized = false;
    unsigned long lastConnectionLogMilliseconds = 0;
    WebServer server = WebServer(80);

    void beginWiFi();
    void beginWebServer();
    void tryConnectStation();
    void handleRoot();
    void handleStatus();
    void handleWifiSave();
    void handleWifiScan();
    void handleProfileGet();
    void handleProfileSave();
    void handleRides();
    void handleReboot();
    void handleNotFound();

    bool sendFile(const char *path, const char *contentType);
    RequestContext detectRequestContext();
    bool isAccessPointClient(const IPAddress &remote) const;
    WifiCredentials loadWifiCredentials() const;
    bool saveWifiCredentials(const String &ssid, const String &password) const;
    void startStationConnection(const String &ssid, const String &password);
    String jsonEscape(const String &value) const;
};

#endif
