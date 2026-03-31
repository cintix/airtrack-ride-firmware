#ifndef HTTP_SERVER_HOST_H
#define HTTP_SERVER_HOST_H

#include <WebServer.h>
#include "WifiManager.h"
#include "ClientApi.h"

class HttpServerHost
{
public:
    HttpServerHost(WifiManager &wifiManager, ClientApi &clientApi);

    void begin();
    void update();
    void stop();
    bool isRunning() const;

private:
    WifiManager &wifi;
    ClientApi &api;
    WebServer server = WebServer(80);
    bool routesRegistered = false;
    bool running = false;

    void registerRoutes();
    void handleRoot();
    void handleNotFound();
    bool sendFile(const char *path, const char *contentType);
};

#endif
