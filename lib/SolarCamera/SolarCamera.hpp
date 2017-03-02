/*
  SolarCamera.hpp - Library for Solar Server.
*/
#ifndef SOLARCAMERA_H
#define SOLARCAMERA_H

#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduCAM.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <ESP8266WebServer.h>
#include "memorysaver.h"

class SolarCamera
{
  public:
    SolarCamera();
    void setup();
    void startCapture();
    void camCapture(ESP8266WebServer& server);
    void serverCapture(ESP8266WebServer& server);
    void serverStream(ESP8266WebServer& server);
  private:
    //set GPIO16 as the slave select
    static const int CS = 16;
  protected:
    ArduCAM myCAM;
};

#endif
