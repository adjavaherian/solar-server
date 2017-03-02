/*
  Server.hpp - Library for Solar Server.
*/
#ifndef SOLARSERVER_H
#define SOLARSERVER_H

#include <ESP8266WebServer.h>
#include <FS.h>
#include <Utils.hpp>
#include <Config.hpp>

class SolarServer
{
  public:
    SolarServer(ESP8266WebServer& server);
    void startRouter();
    bool handleAPConfig();
    void handleClient();
    bool handleConfigPost();
    bool handleConfigReset();
    void handleFileCreate();
    void handleFileDelete();
    void handleEmptyResponse();
    void handleFileList();
    void handleFileUpload();
    bool handleFileRead(String path);
    void handleGetHeap();
    void handleNotFound();
  private:
  protected:
   ESP8266WebServer& server;
};

#endif
