#include "ClientSync.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "../config/Config.h"

namespace
{
    constexpr const char *WIFI_CONFIG_PATH = "/config/wifi.txt";
    constexpr const char *PROFILE_CONFIG_PATH = "/config/profile.txt";

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

void ClientSync::begin()
{
    if (initialized)
    {
        return;
    }

    if (!LittleFS.begin(true))
    {
        Serial.println("Client: LittleFS mount failed");
    }
    else
    {
        Serial.println("Client: LittleFS mounted");
    }

    if (!LittleFS.exists("/config"))
    {
        LittleFS.mkdir("/config");
    }

    beginWiFi();
    beginWebServer();
    initialized = true;
}

void ClientSync::update()
{
    if (!initialized)
    {
        begin();
    }

    server.handleClient();

    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    unsigned long now = millis();
    if ((now - lastConnectionLogMilliseconds) < 10000UL)
    {
        return;
    }

    lastConnectionLogMilliseconds = now;
    Serial.print("Client: STA connected, IP=");
    Serial.println(WiFi.localIP());
}

void ClientSync::beginWiFi()
{
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

    tryConnectStation();
}

void ClientSync::tryConnectStation()
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
        return;
    }

    startStationConnection(credentials.ssid, credentials.password);
}

void ClientSync::startStationConnection(const String &ssid, const String &password)
{
    WiFi.disconnect(true, false);
    delay(100);

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Client: Connecting STA to ");
    Serial.println(ssid);

    const unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startedAt) < WIFI_STA_CONNECT_TIMEOUT_MS)
    {
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Client: STA connected, IP=");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("Client: STA connection timeout");
    }
}

void ClientSync::beginWebServer()
{
    server.on("/", HTTP_GET, [this]() { handleRoot(); });

    server.on("/setup", HTTP_GET, [this]() { sendFile("/www/setup/index.html", "text/html; charset=utf-8"); });
    server.on("/setup/", HTTP_GET, [this]() { sendFile("/www/setup/index.html", "text/html; charset=utf-8"); });
    server.on("/app", HTTP_GET, [this]() { sendFile("/www/app/index.html", "text/html; charset=utf-8"); });
    server.on("/app/", HTTP_GET, [this]() { sendFile("/www/app/index.html", "text/html; charset=utf-8"); });

    server.on("/shared/site.css", HTTP_GET, [this]() { sendFile("/www/shared/site.css", "text/css; charset=utf-8"); });
    server.on("/setup/setup.js", HTTP_GET, [this]() { sendFile("/www/setup/setup.js", "application/javascript; charset=utf-8"); });
    server.on("/app/app.js", HTTP_GET, [this]() { sendFile("/www/app/app.js", "application/javascript; charset=utf-8"); });

    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
    server.on("/api/setup/wifi", HTTP_POST, [this]() { handleWifiSave(); });
    server.on("/api/profile", HTTP_GET, [this]() { handleProfileGet(); });
    server.on("/api/profile", HTTP_POST, [this]() { handleProfileSave(); });
    server.on("/api/rides", HTTP_GET, [this]() { handleRides(); });
    server.on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });

    server.onNotFound([this]() { handleNotFound(); });

    server.begin();
    Serial.println("Client: HTTP server started on port 80");
}

void ClientSync::handleRoot()
{
    RequestContext context = detectRequestContext();
    if (context == RequestContext::AccessPoint)
    {
        sendFile("/www/setup/index.html", "text/html; charset=utf-8");
        return;
    }

    sendFile("/www/app/index.html", "text/html; charset=utf-8");
}

void ClientSync::handleStatus()
{
    RequestContext context = detectRequestContext();

    String json = "{";
    json += "\"context\":\"";
    json += (context == RequestContext::AccessPoint) ? "ap" : "sta";
    json += "\",\"ap_ip\":\"";
    json += WiFi.softAPIP().toString();
    json += "\",\"sta_connected\":";
    json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";
    json += ",\"sta_ip\":\"";
    json += (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
    json += "\",\"uptime_ms\":";
    json += String(millis());
    json += "}";

    server.send(200, "application/json; charset=utf-8", json);
}

void ClientSync::handleWifiScan()
{
    int networkCount = WiFi.scanNetworks(false, true);
    if (networkCount < 0)
    {
        server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"scan_failed\"}");
        return;
    }

    String json = "{\"ok\":true,\"networks\":[";
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
    server.send(200, "application/json; charset=utf-8", json);
}

void ClientSync::handleWifiSave()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    ssid.trim();

    if (ssid.length() == 0)
    {
        server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"ssid_required\"}");
        return;
    }

    if (!saveWifiCredentials(ssid, password))
    {
        server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"persist_failed\"}");
        return;
    }

    startStationConnection(ssid, password);

    String json = "{\"ok\":true,\"sta_connected\":";
    json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";
    json += "}";
    server.send(200, "application/json; charset=utf-8", json);
}

void ClientSync::handleProfileGet()
{
    String weight = readKeyValueFromFile(PROFILE_CONFIG_PATH, "weightKg");
    String age = readKeyValueFromFile(PROFILE_CONFIG_PATH, "ageYears");
    String isMale = readKeyValueFromFile(PROFILE_CONFIG_PATH, "isMale");
    String resting = readKeyValueFromFile(PROFILE_CONFIG_PATH, "restingHeartRateBpm");
    String timezone = readKeyValueFromFile(PROFILE_CONFIG_PATH, "timezoneOffsetMinutes");
    String stopThreshold = readKeyValueFromFile(PROFILE_CONFIG_PATH, "stoppedSpeedThresholdKmh");
    String stopDelay = readKeyValueFromFile(PROFILE_CONFIG_PATH, "stoppedDelaySeconds");

    if (weight.length() == 0)
    {
        weight = "75.0";
    }
    if (age.length() == 0)
    {
        age = "30";
    }
    if (isMale.length() == 0)
    {
        isMale = "1";
    }
    if (resting.length() == 0)
    {
        resting = "60";
    }
    if (timezone.length() == 0)
    {
        timezone = "60";
    }
    if (stopThreshold.length() == 0)
    {
        stopThreshold = "0.0";
    }
    if (stopDelay.length() == 0)
    {
        stopDelay = "0";
    }

    String json = "{";
    json += "\"weightKg\":" + weight;
    json += ",\"ageYears\":" + age;
    json += ",\"isMale\":" + isMale;
    json += ",\"restingHeartRateBpm\":" + resting;
    json += ",\"timezoneOffsetMinutes\":" + timezone;
    json += ",\"stoppedSpeedThresholdKmh\":" + stopThreshold;
    json += ",\"stoppedDelaySeconds\":" + stopDelay;
    json += "}";

    server.send(200, "application/json; charset=utf-8", json);
}

void ClientSync::handleProfileSave()
{
    String weight = server.arg("weightKg");
    String age = server.arg("ageYears");
    String isMale = server.arg("isMale");
    String resting = server.arg("restingHeartRateBpm");
    String timezone = server.arg("timezoneOffsetMinutes");
    String stopThreshold = server.arg("stoppedSpeedThresholdKmh");
    String stopDelay = server.arg("stoppedDelaySeconds");

    File file = LittleFS.open(PROFILE_CONFIG_PATH, "w");
    if (!file || file.isDirectory())
    {
        server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"persist_failed\"}");
        return;
    }

    file.printf("weightKg=%s\n", weight.length() ? weight.c_str() : "75.0");
    file.printf("ageYears=%s\n", age.length() ? age.c_str() : "30");
    file.printf("isMale=%s\n", isMale.length() ? isMale.c_str() : "1");
    file.printf("restingHeartRateBpm=%s\n", resting.length() ? resting.c_str() : "60");
    file.printf("timezoneOffsetMinutes=%s\n", timezone.length() ? timezone.c_str() : "60");
    file.printf("stoppedSpeedThresholdKmh=%s\n", stopThreshold.length() ? stopThreshold.c_str() : "0.0");
    file.printf("stoppedDelaySeconds=%s\n", stopDelay.length() ? stopDelay.c_str() : "0");
    file.close();

    server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

void ClientSync::handleReboot()
{
    server.send(200, "application/json; charset=utf-8", "{\"ok\":true,\"rebooting\":true}");
    delay(200);
    ESP.restart();
}

void ClientSync::handleRides()
{
    const char *json =
        "{"
        "\"today\":{"
        "\"distanceKm\":18.6,"
        "\"durationSeconds\":3262,"
        "\"averageSpeedKmh\":20.5,"
        "\"calories\":612"
        "},"
        "\"rides\":["
        "{"
        "\"id\":219,"
        "\"date\":\"2026-03-31\","
        "\"distanceKm\":26.4,"
        "\"durationSeconds\":4560,"
        "\"avgSpeedKmh\":20.8,"
        "\"points\":[[55.6761,12.5683],[55.6772,12.5602],[55.6819,12.5520],[55.6881,12.5441],[55.6943,12.5365],[55.7005,12.5313],[55.7069,12.5272]]"
        "},"
        "{"
        "\"id\":218,"
        "\"date\":\"2026-03-30\","
        "\"distanceKm\":11.2,"
        "\"durationSeconds\":1860,"
        "\"avgSpeedKmh\":21.6,"
        "\"points\":[[55.6692,12.5451],[55.6674,12.5382],[55.6651,12.5311],[55.6622,12.5240],[55.6598,12.5182],[55.6571,12.5120],[55.6549,12.5059]]"
        "},"
        "{"
        "\"id\":217,"
        "\"date\":\"2026-03-29\","
        "\"distanceKm\":43.1,"
        "\"durationSeconds\":7740,"
        "\"avgSpeedKmh\":20.0,"
        "\"points\":[[55.7031,12.5721],[55.7098,12.5804],[55.7162,12.5887],[55.7230,12.5969],[55.7307,12.6041],[55.7380,12.6090],[55.7443,12.6122]]"
        "}"
        "]"
        "}";

    server.send(200, "application/json; charset=utf-8", json);
}

void ClientSync::handleNotFound()
{
    String uri = server.uri();

    if (uri.startsWith("/setup"))
    {
        sendFile("/www/setup/index.html", "text/html; charset=utf-8");
        return;
    }

    if (uri.startsWith("/app"))
    {
        sendFile("/www/app/index.html", "text/html; charset=utf-8");
        return;
    }

    handleRoot();
}

bool ClientSync::sendFile(const char *path, const char *contentType)
{
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory())
    {
        server.send(404, "text/plain; charset=utf-8", "File not found");
        return false;
    }

    server.streamFile(file, contentType);
    file.close();
    return true;
}

ClientSync::RequestContext ClientSync::detectRequestContext()
{
    IPAddress remote = server.client().remoteIP();
    if (isAccessPointClient(remote))
    {
        return RequestContext::AccessPoint;
    }

    return RequestContext::Station;
}

bool ClientSync::isAccessPointClient(const IPAddress &remote) const
{
    IPAddress apIp = WiFi.softAPIP();
    return (remote[0] == apIp[0] &&
            remote[1] == apIp[1] &&
            remote[2] == apIp[2]);
}

ClientSync::WifiCredentials ClientSync::loadWifiCredentials() const
{
    WifiCredentials credentials;
    credentials.ssid = readKeyValueFromFile(WIFI_CONFIG_PATH, "ssid");
    credentials.password = readKeyValueFromFile(WIFI_CONFIG_PATH, "password");
    return credentials;
}

bool ClientSync::saveWifiCredentials(const String &ssid, const String &password) const
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

String ClientSync::jsonEscape(const String &value) const
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
