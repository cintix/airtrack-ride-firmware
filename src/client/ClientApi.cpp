#include "ClientApi.h"
#include <Arduino.h>

ClientApi::ClientApi(WifiManager &wifiManager, ProfileService &profileService)
    : wifi(wifiManager), profiles(profileService)
{
}

void ClientApi::registerRoutes(WebServer &webServer)
{
    server = &webServer;
    server->on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server->on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
    server->on("/api/setup/wifi", HTTP_POST, [this]() { handleWifiSave(); });
    server->on("/api/profile", HTTP_GET, [this]() { handleProfileGet(); });
    server->on("/api/profile", HTTP_POST, [this]() { handleProfileSave(); });
    server->on("/api/rides", HTTP_GET, [this]() { handleRides(); });
    server->on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });
}

void ClientApi::handleStatus()
{
    const IPAddress remote = server->client().remoteIP();
    const char *context = wifi.isAccessPointClient(remote) ? "ap" : "sta";

    const unsigned long nowMilliseconds = millis();
    const WifiManager::StatusSnapshot status = wifi.getStatusSnapshot(nowMilliseconds);

    String json = "{";
    json += "\"context\":\"";
    json += context;
    json += "\",\"ap_ip\":\"";
    json += status.apIp;
    json += "\",\"sta_connected\":";
    json += status.staConnected ? "true" : "false";
    json += ",\"sta_ip\":\"";
    json += status.staIp;
    json += "\",\"uptime_ms\":";
    json += String(nowMilliseconds);
    json += ",\"wifi_state\":\"";
    json += status.wifiState;
    json += "\",\"retry_attempt\":";
    json += String(status.retryAttempt);
    json += ",\"retry_in_ms\":";
    json += String(status.retryInMilliseconds);
    json += "}";

    server->send(200, "application/json; charset=utf-8", json);
}

void ClientApi::handleWifiScan()
{
    String json;
    if (!wifi.scanNetworksJson(json))
    {
        server->send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"scan_failed\"}");
        return;
    }

    server->send(200, "application/json; charset=utf-8", json);
}

void ClientApi::handleWifiSave()
{
    String ssid = server->arg("ssid");
    String password = server->arg("password");
    ssid.trim();

    if (ssid.length() == 0)
    {
        server->send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"ssid_required\"}");
        return;
    }

    if (!wifi.saveWifiCredentials(ssid, password))
    {
        server->send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"persist_failed\"}");
        return;
    }

    wifi.startStationConnection(ssid, password, true);

    String json = "{\"ok\":true,\"sta_connected\":";
    json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";
    json += "}";
    server->send(200, "application/json; charset=utf-8", json);
}

void ClientApi::handleProfileGet()
{
    server->send(200, "application/json; charset=utf-8", profiles.buildProfileJson());
}

void ClientApi::handleProfileSave()
{
    String weight = server->arg("weightKg");
    String age = server->arg("ageYears");
    String isMale = server->arg("isMale");
    String resting = server->arg("restingHeartRateBpm");
    String timezone = server->arg("timezoneOffsetMinutes");
    String stopThreshold = server->arg("stoppedSpeedThresholdKmh");
    String stopDelay = server->arg("stoppedDelaySeconds");

    if (!profiles.saveProfile(weight, age, isMale, resting, timezone, stopThreshold, stopDelay))
    {
        server->send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"persist_failed\"}");
        return;
    }

    server->send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

void ClientApi::handleReboot()
{
    server->send(200, "application/json; charset=utf-8", "{\"ok\":true,\"rebooting\":true}");
    delay(200);
    ESP.restart();
}

void ClientApi::handleRides()
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

    server->send(200, "application/json; charset=utf-8", json);
}
