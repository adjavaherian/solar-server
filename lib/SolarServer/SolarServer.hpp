/*
  Server.hpp - Library for Solar Server.
*/
#ifndef SOLARSERVER_H
#define SOLARSERVER_H

#include <ESP8266WebServer.h>

class SolarServer
{
  public:
    SolarServer(int port);
    const ESP8266WebServer& getServer();
  private:
    ESP8266WebServer _server;
};

#endif
