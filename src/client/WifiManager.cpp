#include "WifiManager.h"
#include <LittleFS.h>
#include <esp_system.h>
#include <cstring>
#include "../config/Config.h"

namespace
{
    constexpr const char *WIFI_CONFIG_PATH = "/config/wifi.txt";
    constexpr const char *CONFIG_DIRECTORY_PATH = "/config";
    const IPAddress WIFI_AP_IP(WIFI_AP_IP_OCTET_1, WIFI_AP_IP_OCTET_2, WIFI_AP_IP_OCTET_3, WIFI_AP_IP_OCTET_4);
    const IPAddress WIFI_AP_SUBNET(255, 255, 255, 0);

    bool ensureFilesystemReady()
    {
        static bool mounted = false;
        if (!mounted)
        {
            if (!LittleFS.begin(true))
            {
                Serial.println("Client: LittleFS mount failed (wifi config unavailable)");
                return false;
            }

            mounted = true;
        }

        if (!LittleFS.exists(CONFIG_DIRECTORY_PATH))
        {
            if (!LittleFS.mkdir(CONFIG_DIRECTORY_PATH))
            {
                Serial.println("Client: Failed to create /config directory");
                return false;
            }
        }

        return true;
    }

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
        if (!ensureFilesystemReady())
        {
            return "";
        }

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
    lastApDiagnosticsLogMilliseconds = 0;
    stationRetryAttempt = 0;
    stationRetryAtMilliseconds = 0;
    stationConnectStartedMilliseconds = 0;

    WiFi.persistent(false);

    bool shouldUseSta = hasStationCredentialsConfigured();

    uint64_t chipId = ESP.getEfuseMac();
    uint16_t suffix = static_cast<uint16_t>(chipId & 0xFFFF);

    char apSsid[32];
    snprintf(apSsid, sizeof(apSsid), "%s-%04X", WIFI_AP_SSID_PREFIX, suffix);

    bool apStarted = startAccessPointWithReset(apSsid);
    if (!apStarted)
    {
        Serial.println("Client: AP start failed (attempt 1), retrying...");
        delay(WIFI_AP_RETRY_DELAY_MS);
        apStarted = startAccessPointWithReset(apSsid);
    }

    if (!apStarted)
    {
        Serial.println("Client: AP start failed after retry, disabling WiFi state");
        wifiState = WifiState::Disabled;
        return;
    }

    if (shouldUseSta)
    {
        if (!WiFi.mode(WIFI_AP_STA))
        {
            Serial.println("Client: Failed to switch radio mode to AP+STA");
            wifiState = WifiState::ApReadyStaIdle;
            return;
        }

        WiFi.setSleep(false);
        delay(20);
    }

    Serial.print("Client: AP started, SSID=");
    Serial.print(apSsid);
    Serial.print(" mode=");
    Serial.print(shouldUseSta ? "AP+STA" : "AP");
    Serial.print(" channel=");
    Serial.print(WiFi.channel());
    Serial.print(" IP=");
    Serial.println(WiFi.softAPIP());

    if (shouldUseSta)
    {
        tryConnectStation(true);
        return;
    }

    wifiState = WifiState::ApReadyStaIdle;
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

bool WifiManager::startAccessPoint(const char *apSsid)
{
#if WIFI_AP_OPEN_NETWORK
    return WiFi.softAP(apSsid, nullptr, WIFI_AP_CHANNEL, false, WIFI_AP_MAX_CLIENTS);
#else
    const size_t apPasswordLength = strlen(WIFI_AP_PASSWORD);
    if (apPasswordLength < 8 || apPasswordLength > 63)
    {
        Serial.print("Client: Invalid AP password length (");
        Serial.print(apPasswordLength);
        Serial.println("), AP start aborted");
        return false;
    }

    return WiFi.softAP(apSsid, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, false, WIFI_AP_MAX_CLIENTS);
#endif
}

bool WifiManager::startAccessPointWithReset(const char *apSsid)
{
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, false);
    delay(WIFI_AP_RESET_DELAY_MS);

    if (!WiFi.mode(WIFI_MODE_AP))
    {
        Serial.println("Client: Failed to switch radio mode to AP");
        return false;
    }

    WiFi.setSleep(false);

    if (!WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_IP, WIFI_AP_SUBNET))
    {
        Serial.println("Client: AP IP config failed");
    }

    return startAccessPoint(apSsid);
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
    {
        if ((nowMilliseconds - lastApDiagnosticsLogMilliseconds) >= 10000UL)
        {
            lastApDiagnosticsLogMilliseconds = nowMilliseconds;
            Serial.print("Client: AP heartbeat mode=");
            Serial.print(static_cast<int>(WiFi.getMode()));
            Serial.print(" channel=");
            Serial.print(WiFi.channel());
            Serial.print(" stations=");
            Serial.print(WiFi.softAPgetStationNum());
            Serial.print(" IP=");
            Serial.println(WiFi.softAPIP());
        }
        return;
    }

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

bool WifiManager::hasStationCredentialsConfigured() const
{
    WifiCredentials credentials = loadWifiCredentials();

    if (credentials.ssid.length() == 0)
    {
        credentials.ssid = WIFI_STA_SSID;
    }

    return credentials.ssid.length() > 0;
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
    if (!ensureFilesystemReady())
    {
        return false;
    }

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
