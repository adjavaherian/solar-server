/*
  Server.cpp - Library for Solar Server.
*/
#include "SolarServer.hpp"

SolarServer::SolarServer(int port)
{
  ESP8266WebServer _server(port);
}


const ESP8266WebServer& SolarServer::getServer()
{
  return _server;
}
