/*
  Poster.hpp - Posting Library for Solar Server.
*/
#ifndef POSTER_H
#define POSTER_H

#define FS_NO_GLOBALS

#include <ESP8266WiFi.h>
#include <SD.h>

class Poster
{
  public:
    Poster();
    String post(File myFile, String bucket);
  private:
    // post variables
    String post_host = ".s3.amazonaws.com";
    const int post_port = 80;
    String url = "/";
  protected:
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
};

#endif
