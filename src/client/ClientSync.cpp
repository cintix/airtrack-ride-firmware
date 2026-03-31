#include "ClientSync.h"
#include <Arduino.h>
#include <LittleFS.h>

ClientSync::ClientSync()
    : clientApi(wifiManager, profileService),
      httpServer(wifiManager, clientApi)
{
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

    wifiManager.begin(wifiEnabled);

    if (wifiEnabled)
    {
        httpServer.begin();
    }
    else
    {
        Serial.println("Client: WiFi startup skipped (disabled)");
    }

    initialized = true;
}

void ClientSync::update()
{
    if (!initialized)
    {
        begin();
    }

    if (!wifiEnabled)
    {
        return;
    }

    httpServer.update();
    wifiManager.update(millis());
}

void ClientSync::setWifiEnabled(bool enabled)
{
    if (enabled == wifiEnabled)
    {
        return;
    }

    wifiEnabled = enabled;

    if (!initialized)
    {
        return;
    }

    if (wifiEnabled)
    {
        wifiManager.setEnabled(true);
        httpServer.begin();
        return;
    }

    httpServer.stop();
    wifiManager.setEnabled(false);
}
