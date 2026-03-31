#include "HttpServerHost.h"
#include <Arduino.h>
#include <LittleFS.h>

HttpServerHost::HttpServerHost(WifiManager &wifiManager, ClientApi &clientApi)
    : wifi(wifiManager), api(clientApi)
{
}

void HttpServerHost::begin()
{
    if (!routesRegistered)
    {
        registerRoutes();
        routesRegistered = true;
    }

    if (running)
    {
        return;
    }

    server.begin();
    running = true;
    Serial.println("Client: HTTP server started on port 80");
}

void HttpServerHost::update()
{
    if (!running)
    {
        return;
    }

    server.handleClient();
}

void HttpServerHost::stop()
{
    if (!running)
    {
        return;
    }

    server.stop();
    running = false;
    Serial.println("Client: HTTP server stopped");
}

bool HttpServerHost::isRunning() const
{
    return running;
}

void HttpServerHost::registerRoutes()
{
    server.on("/", HTTP_GET, [this]() { handleRoot(); });

    server.on("/setup", HTTP_GET, [this]() { sendFile("/www/setup/index.html", "text/html; charset=utf-8"); });
    server.on("/setup/", HTTP_GET, [this]() { sendFile("/www/setup/index.html", "text/html; charset=utf-8"); });
    server.on("/app", HTTP_GET, [this]() { sendFile("/www/app/index.html", "text/html; charset=utf-8"); });
    server.on("/app/", HTTP_GET, [this]() { sendFile("/www/app/index.html", "text/html; charset=utf-8"); });

    server.on("/shared/site.css", HTTP_GET, [this]() { sendFile("/www/shared/site.css", "text/css; charset=utf-8"); });
    server.on("/setup/setup.js", HTTP_GET, [this]() { sendFile("/www/setup/setup.js", "application/javascript; charset=utf-8"); });
    server.on("/app/app.js", HTTP_GET, [this]() { sendFile("/www/app/app.js", "application/javascript; charset=utf-8"); });

    api.registerRoutes(server);

    server.onNotFound([this]() { handleNotFound(); });
}

void HttpServerHost::handleRoot()
{
    const IPAddress remote = server.client().remoteIP();
    if (wifi.isAccessPointClient(remote))
    {
        sendFile("/www/setup/index.html", "text/html; charset=utf-8");
        return;
    }

    sendFile("/www/app/index.html", "text/html; charset=utf-8");
}

void HttpServerHost::handleNotFound()
{
    const String uri = server.uri();

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

bool HttpServerHost::sendFile(const char *path, const char *contentType)
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
