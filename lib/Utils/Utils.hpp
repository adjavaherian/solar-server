/*
  Utils.hpp - Free function utils.
*/

#ifndef UTILS_H
#define UTILS_H

#include "Arduino.h"

String formatBytes(size_t bytes);
String getContentType(String filename, bool download);

#endif
