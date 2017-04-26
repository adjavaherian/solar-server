/*
  SolarCamera.cpp - Camera controller.
*/

#include <SolarCamera.hpp>

SolarCamera::SolarCamera() : myCAM(OV2640, CS) {
  SolarCamera::setup();
}

void SolarCamera::setup() {
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
  myCAM.OV2640_set_JPEG_size(OV2640_800x600);
  myCAM.clear_fifo_flag();

}

void SolarCamera::startCapture() {
  Serial.println("start_capture");
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}

void SolarCamera::camCapture(ESP8266WebServer& server) {

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

void SolarCamera::serverCapture(ESP8266WebServer& server) {
  SolarCamera::startCapture();
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

  SolarCamera::camCapture(server);

  total_time = millis() - total_time;
  Serial.print("send total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  Serial.println("CAM send Done.");
}

void SolarCamera::serverStream(ESP8266WebServer& server) {

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
    SolarCamera::startCapture();
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

void SolarCamera::captureToSDFile() {

  char str[8];
  byte buf[256];
  static int i = 0;
  static int k = 0;
  static int n = 0;
  uint8_t temp, temp_last;
  File file;

  //Initialize SD Card
  const int SD_CS = 0;
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Error");
  } else {
    Serial.println("SD Card detected!");
  }

  //Flush the FIFO
  myCAM.flush_fifo();

  //Clear the capture done flag
  myCAM.clear_fifo_flag();

  //Start capture
  myCAM.start_capture();
  Serial.println("start capture");

  while(!myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));
  Serial.println("Capture Done!");

  //Construct a file name
  k = k + 1;
  itoa(k, str, 10);
  strcat(str, ".jpg");

  //Open the new file
  file = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);

  if (!file) {
    Serial.println("open file failed");
    return;
  }

  i = 0;
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  #if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
    SPI.transfer(0xFF);
  #endif

  //Read JPEG data from FIFO
  while ( (temp !=0xD9) | (temp_last !=0xFF) ) {
    temp_last = temp;
    temp = SPI.transfer(0x00);

    //Write image data to buffer if not full
    if( i < 256) {
    buf[i++] = temp;
    } else {
    //Write 256 bytes image data to file
    myCAM.CS_HIGH();
    file.write(buf ,256);
    i = 0;
    buf[i++] = temp;
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
    }
    delay(0);

  }

  //Write the remain bytes in the buffer
  if (i > 0) {
    myCAM.CS_HIGH();
    file.write(buf,i);
  }
  //Close the file
  file.close();
  Serial.println("CAM Save Done!");
}
