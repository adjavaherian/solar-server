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
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <SolarServer.hpp>
#include <FS.h>
#include <Config.hpp>
#include <Utils.hpp>
#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>
#include "memorysaver.h"

#define DBG_OUTPUT_PORT Serial

// set GPIO16 as the slave select :
const int CS = 16;

const char* host = "solar-server";
const char* configFile = "config.txt";

// instances for config and server and upload file
Config config(configFile);
ESP8266WebServer server(80);

File fsUploadFile;
ArduCAM myCAM(OV2640, CS);

void start_capture() {
  Serial.println("start_capture");
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}

void camCapture() {

  bool is_header = false;
  static const size_t bufferSize = 4096;
  static uint8_t buffer[bufferSize] = {0xFF};
  uint8_t temp = 0, temp_last = 0;
  uint32_t len  = myCAM.read_fifo_length();

  WiFiClient client = server.client();

  //8M
  if (len >= MAX_FIFO_SIZE) {
    Serial.println(F("Over size."));
  }

  //0 kb
  if (len == 0 ) {
    Serial.println(F("Size is 0."));
  }

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  if (!client.connected()) return;

  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: image/jpeg\r\n";
  response += "Content-len: " + String(len) + "\r\n\r\n";
  server.sendContent(response);

  int i = 0;
  while ( len-- ) {

    temp_last = temp;
    temp =  SPI.transfer(0x00);
    //Read JPEG data from FIFO

    //If find the end ,break while,
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) {
      buffer[i++] = temp;  //save the last  0XD9

      //Write the remaining bytes in the buffer
      if (!client.connected()) break;
      client.write(&buffer[0], i);
      is_header = false;
      i = 0;
      myCAM.CS_HIGH();
      break;
    }

    if (is_header == true) {

      //Write image data to buffer if not full
      if (i < bufferSize) {
        buffer[i++] = temp;
      } else {
        //Write bufferSize bytes image data to file
        if (!client.connected()) break;
        client.write(&buffer[0], bufferSize);
        i = 0;
        buffer[i++] = temp;
      }

    } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
      is_header = true;
      buffer[i++] = temp_last;
      buffer[i++] = temp;
    }
  }
}

void serverCapture() {
  start_capture();
  Serial.println("CAM Capturing");

  int total_time = 0;

  total_time = millis();
  Serial.println("Mark 1");
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  total_time = millis() - total_time;
  Serial.print("capture total_time used (in miliseconds):");
  Serial.println(total_time, DEC);

  total_time = 0;

  Serial.println("CAM Capture Done.");
  total_time = millis();
  camCapture();
  total_time = millis() - total_time;
  Serial.print("send total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  Serial.println("CAM send Done.");
}

void serverStream() {

  uint8_t temp = 0, temp_last = 0;
  static const size_t bufferSize = 4096;
  static uint8_t buffer[bufferSize] = {0xFF};
  int i = 0;
  bool is_header = false;
  WiFiClient client = server.client();

  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (1) {
    start_capture();
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

    size_t len = myCAM.read_fifo_length();

    //8M
    if (len >= MAX_FIFO_SIZE) {
      Serial.println(F("Over size."));
      continue;
    }

    //0 kb
    if (len == 0 ) {
      Serial.println(F("Size is 0."));
      continue;
    }

    myCAM.CS_LOW();
    myCAM.set_fifo_burst();

    if (!client.connected()) break;

    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);

    while ( len-- ) {
      temp_last = temp;
      temp =  SPI.transfer(0x00);

      //Read JPEG data from FIFO
      //If find the end ,break while,
      if ( (temp == 0xD9) && (temp_last == 0xFF) )  {
        buffer[i++] = temp;  //save the last  0XD9
        //Write the remain bytes in the buffer
        myCAM.CS_HIGH();;
        if (!client.connected()) break;
        client.write(&buffer[0], i);
        is_header = false;
        i = 0;
      }

      if (is_header == true) {
        //Write image data to buffer if not full
        if (i < bufferSize) {
          buffer[i++] = temp;
        } else {
          //Write bufferSize bytes image data to file
          myCAM.CS_HIGH();
          if (!client.connected()) break;

          client.write(&buffer[0], bufferSize);
          i = 0;
          buffer[i++] = temp;
          myCAM.CS_LOW();
          myCAM.set_fifo_burst();
        }
      } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
        is_header = true;
        buffer[i++] = temp_last;
        buffer[i++] = temp;
      }

    }
    if (!client.connected()) break;
  }
}

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

  //capture cam
  server.on("/capture", HTTP_GET, serverCapture);

  //stream
  server.on("/stream", HTTP_GET, serverStream);

  //start HTTP server
  server.begin();
  DBG_OUTPUT_PORT.println("solar-server started");
}

void apMode() {
  // AP Init
  Serial.print("Starting AP Mode...");

  // AP to be open with the same host name
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

  // if (String(WiFi.SSID()) != String(ssid)) {
  //
  // }

  Serial.println("new ssid encountered..." + ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // try reconnect 4 times, then apMode.
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    DBG_OUTPUT_PORT.println("waiting for re-connect" + String(WiFi.status()));
    delay(500);
    count++;
    if (count == 10) {
      // WiFi.disconnect();
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

void arduCamSetup() {
  uint8_t vid, pid;
  uint8_t temp;
  #if defined(__SAM3X8E__)
    Wire1.begin();
  #else
    Wire.begin();
  #endif

  // set the CS as an output:
  pinMode(CS, OUTPUT);

  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.println(F("SPI1 interface Error!"));
    while(1);
  }

  #if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
    Serial.println(F("Can't find OV2640 module!"));
  } else {
    Serial.println(F("OV2640 detected."));
  }
  #endif

  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
  myCAM.clear_fifo_flag();

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

  // Arducam setup
  arduCamSetup();

  // disable ssid caching
  // ESP.flashEraseSector(0x3fe);
  // WiFi.persistent(false);
  // WiFi.forceSleepWake();

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
