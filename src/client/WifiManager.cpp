#include "WifiManager.h"
#include <LittleFS.h>
#include <esp_system.h>
#include "../config/Config.h"

namespace
{
    constexpr const char *WIFI_CONFIG_PATH = "/config/wifi.txt";

    bool parseKeyValueLine(const String &line, String &key, String &value)
    {
        int separator = line.indexOf('=');
        if (separator <= 0)
        {
            return false;
        }

        key = line.substring(0, separator);
        value = line.substring(separator + 1);
        key.trim();
        value.trim();
        return key.length() > 0;
    }

    String readKeyValueFromFile(const char *path, const char *targetKey)
    {
        File file = LittleFS.open(path, "r");
        if (!file || file.isDirectory())
        {
            return "";
        }

        while (file.available())
        {
            String line = file.readStringUntil('\n');
            String key;
            String value;
            if (!parseKeyValueLine(line, key, value))
            {
                continue;
            }

            if (key == targetKey)
            {
                file.close();
                return value;
            }
        }

        file.close();
        return "";
    }
}

void WifiManager::begin(bool wifiEnabled)
{
    if (initialized)
    {
        return;
    }

    initialized = true;
    enabled = wifiEnabled;

    if (enabled)
    {
        beginWiFi();
        return;
    }

    wifiState = WifiState::Disabled;
}

void WifiManager::update(unsigned long nowMilliseconds)
{
    if (!initialized || !enabled)
    {
        return;
    }

    updateStateMachine(nowMilliseconds);
}

void WifiManager::setEnabled(bool wifiEnabled)
{
    if (wifiEnabled == enabled)
    {
        return;
    }

    enabled = wifiEnabled;
    Serial.print("Client: WiFi ");
    Serial.println(enabled ? "ENABLED" : "DISABLED");

    if (!initialized)
    {
        return;
    }

    if (enabled)
    {
        beginWiFi();
        return;
    }

    stopWiFi();
}

bool WifiManager::isEnabled() const
{
    return enabled;
}

void WifiManager::beginWiFi()
{
    wifiState = WifiState::Starting;
    stationRetryAttempt = 0;
    stationRetryAtMilliseconds = 0;
    stationConnectStartedMilliseconds = 0;

    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);

    uint64_t chipId = ESP.getEfuseMac();
    uint16_t suffix = static_cast<uint16_t>(chipId & 0xFFFF);

    char apSsid[32];
    snprintf(apSsid, sizeof(apSsid), "%s-%04X", WIFI_AP_SSID_PREFIX, suffix);

    bool apStarted = WiFi.softAP(apSsid, WIFI_AP_PASSWORD);
    if (!apStarted)
    {
        Serial.println("Client: Failed to start AP");
    }
    else
    {
        Serial.print("Client: AP started, SSID=");
        Serial.print(apSsid);
        Serial.print(" IP=");
        Serial.println(WiFi.softAPIP());
    }

    tryConnectStation(true);
}

void WifiManager::stopWiFi()
{
    wifiState = WifiState::Disabled;
    stationRetryAttempt = 0;
    stationRetryAtMilliseconds = 0;
    stationConnectStartedMilliseconds = 0;
    stationSsid = "";
    stationPassword = "";

    WiFi.scanDelete();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    Serial.println("Client: WiFi radio turned off");
}

void WifiManager::tryConnectStation(bool resetRetry)
{
    WifiCredentials credentials = loadWifiCredentials();

    if (credentials.ssid.length() == 0)
    {
        credentials.ssid = WIFI_STA_SSID;
        credentials.password = WIFI_STA_PASSWORD;
    }

    if (credentials.ssid.length() == 0)
    {
        Serial.println("Client: STA skipped (no credentials)");
        wifiState = WifiState::ApReadyStaIdle;
        stationSsid = "";
        stationPassword = "";
        return;
    }

    startStationConnection(credentials.ssid, credentials.password, resetRetry);
}

void WifiManager::startStationConnection(const String &ssid, const String &password, bool resetRetry)
{
    stationSsid = ssid;
    stationPassword = password;

    if (resetRetry)
    {
        stationRetryAttempt = 0;
    }

    WiFi.disconnect(false, false);
    WiFi.begin(stationSsid.c_str(), stationPassword.c_str());
    stationConnectStartedMilliseconds = millis();
    wifiState = WifiState::StaConnecting;

    Serial.print("Client: Connecting STA to ");
    Serial.print(stationSsid);
    Serial.print(" (attempt ");
    Serial.print(stationRetryAttempt + 1);
    Serial.println(")");
}

void WifiManager::updateStateMachine(unsigned long nowMilliseconds)
{
    wl_status_t stationStatus = WiFi.status();

    switch (wifiState)
    {
    case WifiState::Disabled:
        return;

    case WifiState::Starting:
        wifiState = WifiState::ApReadyStaIdle;
        return;

    case WifiState::ApReadyStaIdle:
        return;

    case WifiState::StaConnecting:
        if (stationStatus == WL_CONNECTED)
        {
            stationRetryAttempt = 0;
            wifiState = WifiState::StaConnected;
            lastConnectionLogMilliseconds = 0;
            Serial.print("Client: STA connected, IP=");
            Serial.println(WiFi.localIP());
            return;
        }

        if ((nowMilliseconds - stationConnectStartedMilliseconds) >= WIFI_STA_CONNECT_TIMEOUT_MS)
        {
            scheduleNextStationRetry(nowMilliseconds, "timeout");
        }
        return;

    case WifiState::StaConnected:
        if (stationStatus != WL_CONNECTED)
        {
            scheduleNextStationRetry(nowMilliseconds, "lost");
            return;
        }

        if ((nowMilliseconds - lastConnectionLogMilliseconds) >= WIFI_STA_STATUS_LOG_INTERVAL_MS)
        {
            lastConnectionLogMilliseconds = nowMilliseconds;
            Serial.print("Client: STA connected, IP=");
            Serial.println(WiFi.localIP());
        }
        return;

    case WifiState::StaBackoffWait:
        if (stationStatus == WL_CONNECTED)
        {
            stationRetryAttempt = 0;
            wifiState = WifiState::StaConnected;
            lastConnectionLogMilliseconds = 0;
            Serial.print("Client: STA connected, IP=");
            Serial.println(WiFi.localIP());
            return;
        }

        if (nowMilliseconds >= stationRetryAtMilliseconds)
        {
            startStationConnection(stationSsid, stationPassword, false);
        }
        return;
    }
}

void WifiManager::scheduleNextStationRetry(unsigned long nowMilliseconds, const char *reason)
{
    if (stationSsid.length() == 0)
    {
        wifiState = WifiState::ApReadyStaIdle;
        return;
    }

    if (stationRetryAttempt < 31)
    {
        stationRetryAttempt++;
    }

    uint32_t waitMilliseconds = calculateRetryDelayMilliseconds();
    stationRetryAtMilliseconds = nowMilliseconds + waitMilliseconds;
    wifiState = WifiState::StaBackoffWait;

    Serial.print("Client: STA reconnect scheduled (");
    Serial.print(reason);
    Serial.print("), retry in ");
    Serial.print(waitMilliseconds);
    Serial.print(" ms (attempt ");
    Serial.print(stationRetryAttempt + 1);
    Serial.println(")");
}

uint32_t WifiManager::calculateRetryDelayMilliseconds() const
{
    uint32_t exponent = (stationRetryAttempt > 0) ? (stationRetryAttempt - 1U) : 0U;
    if (exponent > 12)
    {
        exponent = 12;
    }

    uint64_t delayMilliseconds = static_cast<uint64_t>(WIFI_STA_RETRY_BASE_DELAY_MS);
    delayMilliseconds <<= exponent;

    if (delayMilliseconds > WIFI_STA_RETRY_MAX_DELAY_MS)
    {
        delayMilliseconds = WIFI_STA_RETRY_MAX_DELAY_MS;
    }

    uint32_t jitterMilliseconds = 0;
    if (WIFI_STA_RETRY_JITTER_MS > 0)
    {
        jitterMilliseconds = static_cast<uint32_t>(esp_random() % (WIFI_STA_RETRY_JITTER_MS + 1UL));
    }

    uint64_t withJitter = delayMilliseconds + jitterMilliseconds;
    if (withJitter > UINT32_MAX)
    {
        return UINT32_MAX;
    }

    return static_cast<uint32_t>(withJitter);
}

const char *WifiManager::wifiStateText() const
{
    switch (wifiState)
    {
    case WifiState::Disabled:
        return "disabled";
    case WifiState::Starting:
        return "starting";
    case WifiState::ApReadyStaIdle:
        return "ap_ready_sta_idle";
    case WifiState::StaConnecting:
        return "sta_connecting";
    case WifiState::StaConnected:
        return "sta_connected";
    case WifiState::StaBackoffWait:
        return "sta_backoff_wait";
    }

    return "unknown";
}

WifiManager::WifiCredentials WifiManager::loadWifiCredentials() const
{
    WifiCredentials credentials;
    credentials.ssid = readKeyValueFromFile(WIFI_CONFIG_PATH, "ssid");
    credentials.password = readKeyValueFromFile(WIFI_CONFIG_PATH, "password");
    return credentials;
}

bool WifiManager::saveWifiCredentials(const String &ssid, const String &password) const
{
    File file = LittleFS.open(WIFI_CONFIG_PATH, "w");
    if (!file || file.isDirectory())
    {
        return false;
    }

    file.print("ssid=");
    file.println(ssid);
    file.print("password=");
    file.println(password);
    file.close();
    return true;
}

bool WifiManager::scanNetworksJson(String &json) const
{
    int networkCount = WiFi.scanNetworks(false, true);
    if (networkCount < 0)
    {
        return false;
    }

    json = "{\"ok\":true,\"networks\":[";
    for (int index = 0; index < networkCount; index++)
    {
        if (index > 0)
        {
            json += ",";
        }

        json += "{\"ssid\":\"";
        json += jsonEscape(WiFi.SSID(index));
        json += "\",\"rssi\":";
        json += String(WiFi.RSSI(index));
        json += ",\"open\":";
        json += (WiFi.encryptionType(index) == WIFI_AUTH_OPEN) ? "true" : "false";
        json += "}";
    }

    json += "]}";
    WiFi.scanDelete();
    return true;
}

WifiManager::StatusSnapshot WifiManager::getStatusSnapshot(unsigned long nowMilliseconds) const
{
    StatusSnapshot snapshot;
    snapshot.staConnected = (WiFi.status() == WL_CONNECTED);
    snapshot.staIp = snapshot.staConnected ? WiFi.localIP().toString() : "";
    snapshot.apIp = WiFi.softAPIP().toString();
    snapshot.wifiState = wifiStateText();
    snapshot.retryAttempt = stationRetryAttempt;

    if (wifiState == WifiState::StaBackoffWait && stationRetryAtMilliseconds > nowMilliseconds)
    {
        snapshot.retryInMilliseconds = stationRetryAtMilliseconds - nowMilliseconds;
    }

    return snapshot;
}

bool WifiManager::isAccessPointClient(const IPAddress &remote) const
{
    IPAddress apIp = WiFi.softAPIP();
    return (remote[0] == apIp[0] &&
            remote[1] == apIp[1] &&
            remote[2] == apIp[2]);
}

String WifiManager::jsonEscape(const String &value) const
{
    String escaped;
    escaped.reserve(value.length() + 8);

    for (size_t index = 0; index < value.length(); index++)
    {
        char character = value[index];
        switch (character)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += character;
            break;
        }
    }

    return escaped;
}
