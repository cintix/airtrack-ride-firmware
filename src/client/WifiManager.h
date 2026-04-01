#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WifiManager
{
public:
    struct StatusSnapshot
    {
        bool staConnected = false;
        String staIp;
        String apIp;
        const char *wifiState = "disabled";
        uint8_t retryAttempt = 0;
        unsigned long retryInMilliseconds = 0;
    };

    void begin(bool enabled);
    void update(unsigned long nowMilliseconds);
    void setEnabled(bool enabled);
    bool isEnabled() const;

    void startStationConnection(const String &ssid, const String &password, bool resetRetry);
    bool saveWifiCredentials(const String &ssid, const String &password) const;
    bool scanNetworksJson(String &json) const;

    StatusSnapshot getStatusSnapshot(unsigned long nowMilliseconds) const;
    bool isAccessPointClient(const IPAddress &remote) const;

private:
    enum class WifiState
    {
        Disabled,
        Starting,
        ApReadyStaIdle,
        StaConnecting,
        StaConnected,
        StaBackoffWait
    };

    struct WifiCredentials
    {
        String ssid;
        String password;
    };

    bool initialized = false;
    bool enabled = true;
    unsigned long lastConnectionLogMilliseconds = 0;
    unsigned long lastApDiagnosticsLogMilliseconds = 0;
    unsigned long stationConnectStartedMilliseconds = 0;
    unsigned long stationRetryAtMilliseconds = 0;
    uint8_t stationRetryAttempt = 0;
    WifiState wifiState = WifiState::Disabled;
    String stationSsid;
    String stationPassword;

    void beginWiFi();
    void stopWiFi();
    void tryConnectStation(bool resetRetry);
    void updateStateMachine(unsigned long nowMilliseconds);
    void scheduleNextStationRetry(unsigned long nowMilliseconds, const char *reason);
    uint32_t calculateRetryDelayMilliseconds() const;
    const char *wifiStateText() const;
    bool hasStationCredentialsConfigured() const;
    WifiCredentials loadWifiCredentials() const;
    String jsonEscape(const String &value) const;
};

#endif
