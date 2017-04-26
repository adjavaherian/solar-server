/*
  Utils.hpp - Free function utils.
*/

#ifndef UTILS_H
#define UTILS_H

extern "C" {
  #include "user_interface.h"
}

#include "Arduino.h"

String formatBytes(size_t bytes);
String getContentType(String filename, bool download);
void wakeup();
void sleepNow();

#endif
