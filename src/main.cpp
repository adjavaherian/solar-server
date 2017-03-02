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

// set GPIO16 as the slave select :
const int CS = 16;

const char* host = "solar-server";

// instances for config and server and upload file
Config config;
ESP8266WebServer srv(80);
SolarServer server(srv);
ArduCAM myCAM(OV2640, CS);

// void start_capture() {
//   Serial.println("start_capture");
//   myCAM.clear_fifo_flag();
//   myCAM.start_capture();
// }
//
// void camCapture() {
//
//   bool is_header = false;
//   static const size_t bufferSize = 4096;
//   static uint8_t buffer[bufferSize] = {0xFF};
//   uint8_t temp = 0, temp_last = 0;
//   uint32_t len  = myCAM.read_fifo_length();
//
//   WiFiClient client = server.client();
//
//   //8M
//   if (len >= MAX_FIFO_SIZE) {
//     Serial.println(F("Over size."));
//   }
//
//   //0 kb
//   if (len == 0 ) {
//     Serial.println(F("Size is 0."));
//   }
//
//   myCAM.CS_LOW();
//   myCAM.set_fifo_burst();
//
//   if (!client.connected()) return;
//
//   String response = "HTTP/1.1 200 OK\r\n";
//   response += "Content-Type: image/jpeg\r\n";
//   response += "Content-len: " + String(len) + "\r\n\r\n";
//   server.sendContent(response);
//
//   int i = 0;
//   while ( len-- ) {
//
//     temp_last = temp;
//     temp =  SPI.transfer(0x00);
//     //Read JPEG data from FIFO
//
//     //If find the end ,break while,
//     if ( (temp == 0xD9) && (temp_last == 0xFF) ) {
//       buffer[i++] = temp;  //save the last  0XD9
//
//       //Write the remaining bytes in the buffer
//       if (!client.connected()) break;
//       client.write(&buffer[0], i);
//       is_header = false;
//       i = 0;
//       myCAM.CS_HIGH();
//       break;
//     }
//
//     if (is_header == true) {
//
//       //Write image data to buffer if not full
//       if (i < bufferSize) {
//         buffer[i++] = temp;
//       } else {
//         //Write bufferSize bytes image data to file
//         if (!client.connected()) break;
//         client.write(&buffer[0], bufferSize);
//         i = 0;
//         buffer[i++] = temp;
//       }
//
//     } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
//       is_header = true;
//       buffer[i++] = temp_last;
//       buffer[i++] = temp;
//     }
//   }
// }
//
// void serverCapture() {
//   start_capture();
//   Serial.println("CAM Capturing");
//
//   int total_time = 0;
//
//   total_time = millis();
//   Serial.println("Mark 1");
//   while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
//   total_time = millis() - total_time;
//   Serial.print("capture total_time used (in miliseconds):");
//   Serial.println(total_time, DEC);
//
//   total_time = 0;
//
//   Serial.println("CAM Capture Done.");
//   total_time = millis();
//   camCapture();
//   total_time = millis() - total_time;
//   Serial.print("send total_time used (in miliseconds):");
//   Serial.println(total_time, DEC);
//   Serial.println("CAM send Done.");
// }
//
// void serverStream() {
//
//   uint8_t temp = 0, temp_last = 0;
//   static const size_t bufferSize = 4096;
//   static uint8_t buffer[bufferSize] = {0xFF};
//   int i = 0;
//   bool is_header = false;
//   WiFiClient client = server.client();
//
//   String response = "HTTP/1.1 200 OK\r\n";
//   response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
//   server.sendContent(response);
//
//   while (1) {
//     start_capture();
//     while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
//
//     size_t len = myCAM.read_fifo_length();
//
//     //8M
//     if (len >= MAX_FIFO_SIZE) {
//       Serial.println(F("Over size."));
//       continue;
//     }
//
//     //0 kb
//     if (len == 0 ) {
//       Serial.println(F("Size is 0."));
//       continue;
//     }
//
//     myCAM.CS_LOW();
//     myCAM.set_fifo_burst();
//
//     if (!client.connected()) break;
//
//     response = "--frame\r\n";
//     response += "Content-Type: image/jpeg\r\n\r\n";
//     server.sendContent(response);
//
//     while ( len-- ) {
//       temp_last = temp;
//       temp =  SPI.transfer(0x00);
//
//       //Read JPEG data from FIFO
//       //If find the end ,break while,
//       if ( (temp == 0xD9) && (temp_last == 0xFF) )  {
//         buffer[i++] = temp;  //save the last  0XD9
//         //Write the remain bytes in the buffer
//         myCAM.CS_HIGH();;
//         if (!client.connected()) break;
//         client.write(&buffer[0], i);
//         is_header = false;
//         i = 0;
//       }
//
//       if (is_header == true) {
//         //Write image data to buffer if not full
//         if (i < bufferSize) {
//           buffer[i++] = temp;
//         } else {
//           //Write bufferSize bytes image data to file
//           myCAM.CS_HIGH();
//           if (!client.connected()) break;
//
//           client.write(&buffer[0], bufferSize);
//           i = 0;
//           buffer[i++] = temp;
//           myCAM.CS_LOW();
//           myCAM.set_fifo_burst();
//         }
//       } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
//         is_header = true;
//         buffer[i++] = temp_last;
//         buffer[i++] = temp;
//       }
//
//     }
//     if (!client.connected()) break;
//   }
// }


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
  server.startRouter();
  // need to write a router override public method
  // server.on("/", handleAPConfig);

  Serial.println("HTTP server started in AP mode");
}

void clientMode(String ssid, String password) {


  //WIFI INIT
  Serial.println("Connecting to: " + ssid);

  Serial.println("currently configured SSID: " + String(WiFi.SSID()));

  // if (String(WiFi.SSID()) != String(ssid)) {
  //
  // }

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
    if (count == 10) {
      // WiFi.disconnect();
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
  server.startRouter();

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

  Serial.begin(115200);
  Serial.println();
  Serial.setDebugOutput(true);

  // SPIFFS setup
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
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
