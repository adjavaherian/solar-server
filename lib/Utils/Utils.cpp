/*
  Utils.cpp - Free function utils.
*/
#include "Utils.hpp"

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename, bool download){
  if(download) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void wakeup(void) {
  wifi_fpm_close;
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
  Serial.println("Woke up from sleep");
}

void sleepNow() {
  Serial.println("going to light sleep...");
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); //light sleep mode

  wifi_fpm_open();
  delay(100);
  wifi_fpm_set_wakeup_cb(wakeup); //wakeup callback
  wifi_fpm_do_sleep(10 * 1000);
  delay(100);
}
