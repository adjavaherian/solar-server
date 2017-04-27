/*
  Post multipart form-data
 */

#include <Poster.hpp>

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8

Poster::Poster() {};

void Poster::post(File myFile) {

  // open sd and file
  // String fileName = path;
  // SD.begin(0);
  // fs::File myFile = SD.open(fileName);
  String fileName = myFile.name();
  String fileSize = String(myFile.size());

  Serial.println();
  Serial.println("file exists");
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
