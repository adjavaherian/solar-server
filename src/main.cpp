/**
  Solar Server
  esp8266 with access point mode and client ssid config

  Based on FSWebServer
  https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer/examples/FSBrowser/FSBrowser.ino

  access the sample web page at http://solar-server.local
  edit the page by going to http://solar-server.local/edit
*/

#ifndef UNIT_TEST  // IMPORTANT LINE!

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SolarServer.hpp>
#include <Config.hpp>
#include <Utils.hpp>
#include <ESP8266HTTPClient.h>


//temp
#include <Arduino.h>
#include <SD.h>
#include <SDController.hpp>

// host name
const char* host = "solar-server";

// instances for config and server
Config config;
SolarServer server(80);
SDController sdc;

void apMode() {
  // AP Init
  Serial.print("Starting AP Mode...");

  // AP to be open with the same host name
  WiFi.mode(WIFI_AP);
  WiFi.softAP(host);
  IPAddress myIP = WiFi.softAPIP();

  Serial.print("AP IP address: " + myIP);
  Serial.println();

  // mdns helper
  MDNS.begin(host);

  // start server and have the server handle AP config on default route
  String indexPath = "/config.htm";
  server.startRouter(indexPath);

  Serial.println("HTTP server started in AP mode");
}

void clientMode(String ssid, String password) {

  //WIFI INIT
  Serial.println("Connecting to: " + ssid);

  Serial.println("currently configured SSID: " + String(WiFi.SSID()));

  Serial.println("new ssid encountered..." + ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // mdns helper
  MDNS.begin(host);

  // try reconnect 4 times, then apMode.
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("waiting for re-connect" + String(WiFi.status()));
    delay(500);
    count++;
    if (count == 20) {
      return apMode();
    }
  }

  // debug
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Open http://");
  Serial.print(host);
  Serial.println(".local to see the file browser");

  // start the server
  String indexPath = "/";
  server.startRouter(indexPath);

}

void setup(void) {

  // Serial Init
  Serial.begin(115200);
  Serial.println();
  Serial.setDebugOutput(true);

  sdc.init();

  // SPIFFS setup
  SPIFFS.begin();
  {
    fs::Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }

  // disable ssid caching // debug
  // WiFi.persistent(false);

  // if no config.txt exists, run AP mode, else run in client mode
  if ( !config.exists() ) {

    apMode();

  } else {

    Serial.println("attempting config read...");
    config.readConfigSettings();
    Serial.print("SSID=");

    String ssid = config.ssid();
    String password = config.password();
    String customHost = config.host();

    Serial.println(config.ssid());

    if (ssid.length() == 0) {
      apMode();
    } else {
      clientMode(ssid, password);
    }

  }

}

void loop(void) {

  server.handleClient();

}

#endif
