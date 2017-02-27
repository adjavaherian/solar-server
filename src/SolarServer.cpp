/*

  Solar Server
  esp8266 with access point mode and client ssid config

  Based on FSWebServer
  https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer/examples/FSBrowser/FSBrowser.ino

  access the sample web page at http://solar-server.local
  edit the page by going to http://solar-server.local/edit
*/

#ifndef UNIT_TEST  // IMPORTANT LINE!

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Config.hpp>
#include <Utils.hpp>

#define DBG_OUTPUT_PORT Serial

const char* host = "solar-server";
const char* configFile = "config.txt";

// instances for config and server and upload file
Config config(configFile);
ESP8266WebServer server(80);
File fsUploadFile;

bool handleFileRead(String path) {

  DBG_OUTPUT_PORT.println("handleFileRead: " + path);

  if (path.endsWith("/")) path += "index.htm";

  String contentType = getContentType(path, server.hasArg("download"));
  String pathWithGz = path + ".gz";

  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {

    if (SPIFFS.exists(pathWithGz)) path += ".gz";

    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;

  }

  return false;

}

void handleFileUpload() {
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}

bool handleAPConfig() {
	// server.send(200, "text/html", "<h1>You are connected</h1>");
  String configRoute = "/config.htm";
  return handleFileRead(configRoute);

}

bool handleConfigPost() {

  String tryAgain = "/try_again.htm";
  String success = "/success.htm";
  String SSID = "SSID";
  String PASSWORD = "PASSWORD";

  if(server.args() == 0)
    return handleFileRead(tryAgain);

  String client_ssid = server.arg(0);
  String client_password = server.arg(1);
  DBG_OUTPUT_PORT.println("handleConfigPost: " + client_ssid + ":" + client_password);

  if(client_ssid.length() == 0)
    return handleFileRead(tryAgain);

  // apply the new settings to the config class
  // and write them to file
  config.applyConfigSetting(SSID, client_ssid);
  config.applyConfigSetting(PASSWORD, client_password);

  if(config.writeConfigSettings()) {
    return handleFileRead(success);
    //restart here
  }

}

bool handleConfigReset() {

  String reset = "/reset.htm";
  String tryAgain = "/try_again.htm";

  DBG_OUTPUT_PORT.println("handleConfigReset...");

  config.resetConfigSettings();
  if (config.writeConfigSettings()) {
    return handleFileRead(reset);
    //restart here
  }
  return handleFileRead(tryAgain);

}


void startServer() {

  Serial.println("starting router");

  //mdns helper
  MDNS.begin(host);

  //server routing
  server.on("/list", HTTP_GET, handleFileList);

  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });

  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);

  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);

  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"analog\":"+String(analogRead(A0));
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });

  //get config form posts
  server.on("/config", HTTP_POST, handleConfigPost);

  //handle config reset
  server.on("/reset", HTTP_POST, handleConfigReset);

  //start HTTP server
  server.begin();
  DBG_OUTPUT_PORT.println("solar-server started");
}


void apMode() {
  // AP Init
  Serial.print("Starting AP Mode...");

  /* AP to be open with the same host name */
  WiFi.mode(WIFI_AP);
  WiFi.softAP(host);

  IPAddress myIP = WiFi.softAPIP();

  Serial.print("AP IP address: " + myIP);
  Serial.println();

  // start server and have the server handle AP config on default route
  startServer();
  server.on("/", handleAPConfig);

  Serial.println("HTTP server started in AP mode");
}

void clientMode(String ssid, String password) {


  //WIFI INIT
  DBG_OUTPUT_PORT.println("Connecting to: " + ssid);

  Serial.println("currently configured SSID: " + String(WiFi.SSID()));

  if (String(WiFi.SSID()) != String(ssid)) {
    Serial.println("new ssid encountered...");
    WiFi.begin(ssid.c_str(), password.c_str());
  }

  // try reconnect 4 times, then apMode.
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    DBG_OUTPUT_PORT.println("waiting for re-connect" + String(WiFi.status()));
    delay(500);
    count++;
    if (count == 10) {
      WiFi.disconnect();
      return apMode();
    }
  }

  // debug
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.println(".local to see the file browser");

  // start the server
  startServer();

}

void setup(void) {

  // Serial Init

  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.println();
  DBG_OUTPUT_PORT.setDebugOutput(true);

  // SPIFFS setup
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }

  // disable ssid caching
  ESP.flashEraseSector(0x3fe);
  WiFi.persistent(false);

  // if no config.txt exists, run AP mode, else run in client mode
  if ( !config.exists() ) {

    apMode();

  } else {

    Serial.println("attempting config read...");
    config.readConfigSettings();
    Serial.print("SSID=");

    String ssid = config.ssid();
    String password = config.password();

    Serial.println(config.ssid());

    if (ssid.length() == 0) {
      apMode();
    } else {
      clientMode(ssid, password);
    }

  }

}

void loop(void){
  server.handleClient();
}

#endif
