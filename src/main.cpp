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

const char* host = "solar-server";

// post variables
const char* post_host = "solar-server.s3.amazonaws.com";
const int post_port = 80  ;
String url = "/";


// instances for config and server
Config config;
SolarServer server(80);
SDController sdc;
// Use WiFiClient class to create TCP connections
WiFiClient client;

#define USE_SERIAL Serial

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
  // WiFi.forceSleepWake();
  // return apMode();

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

  // server.handleClient();


  Serial.println("DELAY-----");
  delay(10000);

  // open sd and file
  String fileName = "2.JPG";
  SD.begin(0);
  File myFile = SD.open(fileName);
  String fileSize = String(myFile.size());

  Serial.println();
  Serial.println("bernie file exists");
  Serial.println(myFile);

  if (myFile) {

    // print content length and host
    Serial.println("contentLength");
    Serial.println(fileSize);
    Serial.print("connecting to ");
    Serial.println(post_host);

    // try connect or return on fail
    if (!client.connect(post_host, post_port)) {
      Serial.println("http post connection failed");
      return;
    }

    // We now create a URI for the request
    Serial.println("Connected to server");
    Serial.print("Requesting URL: ");
    Serial.println(url);


    // Make a HTTP request and add HTTP headers
    String boundary = "SolarServerBoundaryjg2qVIUS8teOAbN3";
    String contentType = "image/jpeg";
    String portString = String(post_port);
    String hostString = String(post_host);

    // post header
    String postHeader = "POST " + url + " HTTP/1.1\r\n";
    postHeader += "Host: " + hostString + ":" + portString + "\r\n";
    postHeader += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
    postHeader += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    postHeader += "Accept-Encoding: gzip,deflate\r\n";
    postHeader += "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n";
    postHeader += "User-Agent: Arduino/Solar-Server\r\n";
    postHeader += "Keep-Alive: 300\r\n";
    postHeader += "Connection: keep-alive\r\n";
    postHeader += "Accept-Language: en-us\r\n";

    // key header
    String keyHeader = "--" + boundary + "\r\n";
    keyHeader += "Content-Disposition: form-data; name=\"key\"\r\n\r\n";
    keyHeader += "${filename}\r\n";

    // request header
    String requestHead = "--" + boundary + "\r\n";
    requestHead += "Content-Disposition: form-data; name=\"file\"; filename=\"" + fileName + "\"\r\n";
    requestHead += "Content-Type: " + contentType + "\r\n\r\n";

    // request tail
    String tail = "\r\n--" + boundary + "--\r\n\r\n";

    // content length
    int contentLength = keyHeader.length() + requestHead.length() + myFile.size() + tail.length();
    postHeader += "Content-Length: " + String(contentLength, DEC) + "\n\n";

    // send post header
    char charBuf0[postHeader.length() + 1];
    postHeader.toCharArray(charBuf0, postHeader.length() + 1);
    client.write(charBuf0);
    Serial.print(charBuf0);

    // send key header
    char charBufKey[keyHeader.length() + 1];
    keyHeader.toCharArray(charBufKey, keyHeader.length() + 1);
    client.write(charBufKey);
    Serial.print(charBufKey);

    // send request buffer
    char charBuf1[requestHead.length() + 1];
    requestHead.toCharArray(charBuf1, requestHead.length() + 1);
    client.write(charBuf1);
    Serial.print(charBuf1);

    // create buffer
    const int bufSize = 2048;
    byte clientBuf[bufSize];
    int clientCount = 0;

    while (myFile.available()) {

      clientBuf[clientCount] = myFile.read();

      clientCount++;

      if (clientCount > (bufSize - 1)) {
        client.write((const uint8_t *)clientBuf, bufSize);
        clientCount = 0;
      }

    }

    if (clientCount > 0) {
      client.write((const uint8_t *)clientBuf, clientCount);
      Serial.println("Sent LAST buffer");
    }

    // send tail
    char charBuf3[tail.length() + 1];
    tail.toCharArray(charBuf3, tail.length() + 1);
    client.write(charBuf3);
    Serial.print(charBuf3);



    // Read all the lines of the reply from server and print them to Serial
    Serial.println("request sent");
    while (client.connected()) {
      // Serial.println("while client connected");
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }

    while (client.connected()) {
      // Serial.println("while client connected");
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r") {
        Serial.println("body response received");
        break;
      }
    }
    String line = client.readStringUntil('\n');

    Serial.println("reply was:");
    Serial.println("==========");
    Serial.println(line);
    Serial.println("==========");
    Serial.println("closing connection");


    // close the file:
    myFile.close();


  }


}

#endif
