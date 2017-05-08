/*
  Server.cpp - Router and handlers for Solar Server.
  Wraps ESP8266WebServer instance
*/

#include "SolarServer.hpp"

SolarServer::SolarServer(int port, Config& config): _config(config), server(port){}

String SolarServer::getNameParam() {
  // check name param
  String name = "";
  String params = "";
  for (uint8_t i=0; i<server.args(); i++){
    params += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    Serial.println("params");
    Serial.println(params);
    if (server.argName(i).equals("name")) {
      name += server.arg(i);
    }
  }

  return name;
}

bool SolarServer::handleAPConfig() {
  // server.send(200, "text/html", "<h1>You are connected</h1>");
  String configRoute = "/config.htm";
  return handleFileRead(configRoute);
}

void SolarServer::handleClient() {
  server.handleClient();
}

bool SolarServer::handleConfigPost() {

  String tryAgain = "/try_again.htm";
  String success = "/success.htm";
  String SSID = "SSID";
  String PASSWORD = "PASSWORD";
  String HOST = "HOST";

  if(server.args() == 0)
    return handleFileRead(tryAgain);

  String client_ssid = server.arg(0);
  String client_password = server.arg(1);
  String client_host = server.arg(2);
  Serial.println("handleConfigPost: " + client_ssid + ":" + client_password + ":" + client_host);

  if(client_ssid.length() == 0)
    return handleFileRead(tryAgain);

  // apply the new settings to the config class
  // and write them to file
  _config.applyConfigSetting(SSID, client_ssid);
  _config.applyConfigSetting(PASSWORD, client_password);
  _config.applyConfigSetting(HOST, client_host);

  if(_config.writeConfigSettings()) {
    return handleFileRead(success);
    //restart here
  }

}

bool SolarServer::handleConfigReset() {

  String tryAgain = "/try_again.htm";

  Serial.println("handleConfigReset...");

  _config.resetConfigSettings();
  if (_config.writeConfigSettings()) {
    return server.send(200, "text/plain", "success");
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
  fs::File file = SPIFFS.open(path, "w");
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

    fs::File file = SPIFFS.open(path, "r");
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
  fs::Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    fs::File entry = dir.openFile("r");
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

  fs::File fsUploadFile;

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
  json += ", \"ip\":" + String(" \"") + _config.ip() + String("\"");
  json += ", \"host\":" + String(" \"") + _config.host() + String("\"");
  json += ", \"ssid\":" + String(" \"") + _config.ssid() + String("\"");
  json += "}";
  server.send(200, "text/json", json);
  json = String();
}

void SolarServer::handleServerCapture() {
  Serial.println("handleServerCapture");
  SolarCamera sc;
  return sc.serverCapture(server);
}

void SolarServer::handleServerStream() {
  Serial.println("handleServerStream");
  SolarCamera sc;
  return sc.serverStream(server);
}

void SolarServer::handleCaptureFile() {
  // get name param
  String name = SolarServer::getNameParam();

  // capture file with name
  Serial.println("handle file capture");
  SolarCamera sc;
  sc.captureToSDFile(name);
  return server.send(200, "text/plain", "captured " + name + ".jpg");
}

void SolarServer::handlePostFile() {
  // get name param
  String name = SolarServer::getNameParam();

  if (!name.endsWith("jpg")) name += ".jpg";

  // post file
  Poster poster;
  File myFile = SD.open(name);
  poster.post(myFile);
  return server.send(200, "text/plain", "posted " + name);
}

void SolarServer::handleDeepSleep() {
  //TODO make this configurable
  ESP.deepSleep(10 * 1000000, WAKE_RF_DEFAULT);
}

void SolarServer::handleSleep() {

  // get name param
  String name = SolarServer::getNameParam();
  if (!name.endsWith("jpg")) name += ".jpg";

  // send 200 first
  Serial.println("starting a sleep cycle...");
  server.send(200, "text/plain", "begin capture, post, sleep, wake cycle for file: " + name);

  _shouldSleep = true;

  while(_shouldSleep) {

    // capture
    Serial.println("capture, post, sleep, wake cycle");
    SolarCamera sc;
    sc.captureToSDFile(name);
    delay(5000);

    // post
    Poster poster;
    String path = String(name);
    Serial.println(path);
    File myFile = SD.open(path);
    poster.post(myFile);
    delay(5000);

    // sleep
    Serial.println("Light sleep:");
    sleepNow();
    delay(20000);

    // wake
    Serial.println("Awake from sleep:");
    wakeup();
    delay(10000);

  }

}

void SolarServer::handleWake() {
  // set bool
  _shouldSleep = false;
  // send 200 first
  Serial.println("ending a sleep cycle...");
  server.send(200, "text/plain", "ending sleep, wake cycle...");
}

void SolarServer::startRouter(String indexPath) {

  Serial.println("starting router with index " + indexPath);

  server.on("/", HTTP_GET, std::bind(&SolarServer::handleFileRead, this, indexPath));

  //server routing
  server.on("/list", HTTP_GET, std::bind(&SolarServer::handleFileList, this));

  //load editor
  server.on("/edit", HTTP_GET, std::bind(&SolarServer::handleFileRead, this, "/edit.htm"));

  //create file
  server.on("/edit", HTTP_PUT, std::bind(&SolarServer::handleFileCreate, this));

  //delete file
  server.on("/edit", HTTP_DELETE, std::bind(&SolarServer::handleFileDelete, this));

  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, std::bind(&SolarServer::handleEmptyResponse, this), std::bind(&SolarServer::handleFileUpload, this));

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, std::bind(&SolarServer::handleGetHeap, this));

  //get config form posts
  server.on("/config", HTTP_POST, std::bind(&SolarServer::handleConfigPost, this));

  //handle config reset
  server.on("/reset", HTTP_POST, std::bind(&SolarServer::handleConfigReset, this));

  // capture cam
  server.on("/capture", HTTP_GET, std::bind(&SolarServer::handleServerCapture, this));

  //stream
  server.on("/stream", HTTP_GET, std::bind(&SolarServer::handleServerStream, this));

  //capture file
  server.on("/capture-file", HTTP_GET, std::bind(&SolarServer::handleCaptureFile, this));

  //post file
  server.on("/post-file", HTTP_GET, std::bind(&SolarServer::handlePostFile, this));

  //start sleeping
  server.on("/sleep", HTTP_GET, std::bind(&SolarServer::handleSleep, this));

  //deep sleep
  server.on("/deep-sleep", HTTP_GET, std::bind(&SolarServer::handleDeepSleep, this));

  //awake
  server.on("/wake", HTTP_GET, std::bind(&SolarServer::handleWake, this));

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound(std::bind(&SolarServer::handleNotFound, this));

  //start HTTP server
  server.begin();
  Serial.println("solar-server started");
}
