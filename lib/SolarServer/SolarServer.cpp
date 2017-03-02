/*
  Server.cpp - Router and handlers for Solar Server.
  Wraps ESP8266WebServer instance
*/

#include "SolarServer.hpp"

SolarServer::SolarServer(int port): server(port){}

bool SolarServer::handleAPConfig() {
  // server.send(200, "text/html", "<h1>You are connected</h1>");
  String configRoute = "/config.htm";
  return handleFileRead(configRoute);
}

void SolarServer::handleClient() {
  server.handleClient();
}

bool SolarServer::handleConfigPost() {

  Config config;
  String tryAgain = "/try_again.htm";
  String success = "/success.htm";
  String SSID = "SSID";
  String PASSWORD = "PASSWORD";

  if(server.args() == 0)
    return handleFileRead(tryAgain);

  String client_ssid = server.arg(0);
  String client_password = server.arg(1);
  Serial.println("handleConfigPost: " + client_ssid + ":" + client_password);

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

bool SolarServer::handleConfigReset() {

  Config config;
  String reset = "/reset.htm";
  String tryAgain = "/try_again.htm";

  Serial.println("handleConfigReset...");

  config.resetConfigSettings();
  if (config.writeConfigSettings()) {
    return handleFileRead(reset);
    //restart here
  }
  return handleFileRead(tryAgain);

}

void SolarServer::handleFileCreate() {
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
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

void SolarServer::handleFileDelete() {
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void SolarServer::handleEmptyResponse() {
  server.send(200, "text/plain", "");
}

bool SolarServer::handleFileRead(String path) {

  // String path = server.uri();
  Serial.println("handleFileRead: " + path);

  if (path.endsWith("/")) path += "index.htm";

  String contentType = getContentType(path, server.hasArg("download"));
  String pathWithGz = path + ".gz";

  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {

    if (SPIFFS.exists(pathWithGz)) path += ".gz";

    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();

    return true;

  } else {

    server.send(404, "text/plain", "FileNotFound");
    return false;

  }

}

void SolarServer::handleFileList() {

  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}

  String path = server.arg("dir");
  Serial.println("handleFileList: " + path);
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

void SolarServer::handleFileUpload() {

  File fsUploadFile;

  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();

  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void SolarServer::handleNotFound() {
  if(!handleFileRead(server.uri()))
    server.send(404, "text/plain", "FileNotFound");
}

void SolarServer::handleGetHeap() {
  String json = "{";
  json += "\"heap\":"+String(ESP.getFreeHeap());
  json += ", \"analog\":"+String(analogRead(A0));
  json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
  json += "}";
  server.send(200, "text/json", json);
  json = String();
}

void SolarServer::startRouter() {

  String edit = "/edit.htm";

  Serial.println("starting router");

  //server routing
  server.on("/list", HTTP_GET, std::bind(&SolarServer::handleFileList, this));

  //load editor
  server.on("/edit", HTTP_GET, std::bind(&SolarServer::handleFileRead, this, edit));

  //create file
  server.on("/edit", HTTP_PUT, std::bind(&SolarServer::handleFileCreate, this));

  //delete file
  server.on("/edit", HTTP_DELETE, std::bind(&SolarServer::handleFileDelete, this));

  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, std::bind(&SolarServer::handleEmptyResponse, this), std::bind(&SolarServer::handleFileUpload, this));

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound(std::bind(&SolarServer::handleNotFound, this));

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, std::bind(&SolarServer::handleGetHeap, this));

  //get config form posts
  server.on("/config", HTTP_POST, std::bind(&SolarServer::handleConfigPost, this));

  //handle config reset
  server.on("/reset", HTTP_POST, std::bind(&SolarServer::handleConfigReset, this));
  //
  //capture cam
  // server.on("/capture", HTTP_GET, std::bind(&SolarCamera::serverCapture, this, server));
  //
  // //stream
  // server.on("/stream", HTTP_GET, serverStream);

  //start HTTP server
  server.begin();
  Serial.println("solar-server started");
}
