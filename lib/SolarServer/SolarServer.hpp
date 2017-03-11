/*
  Server.hpp - Library for Solar Server.
*/
#ifndef SOLARSERVER_H
#define SOLARSERVER_H
#define FS_NO_GLOBALS

#include <ESP8266WebServer.h>
#include <FS.h>
#include <SolarCamera.hpp>
#include <Utils.hpp>
#include <Config.hpp>

class SolarServer
{
  public:
    SolarServer(int port);
    void startRouter(String index);
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
    void handleServerCapture();
    void handleServerStream();
    void handleCaptureFile();
  private:
  protected:
   ESP8266WebServer server;
};

#endif
