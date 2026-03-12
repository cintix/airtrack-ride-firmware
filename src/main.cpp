#include "gps/GpsReader.h"
#include "application/Application.h"
#include "storage/Storage.h"
#include "client/ClientSync.h"

GpsReader gps;
Application application;
Storage storage;
ClientSync client;

void setup()
{
    gps.begin();
    storage.begin();
}

void loop()
{
    gps.update();
    application.update();
    storage.update();
    client.update();
}
