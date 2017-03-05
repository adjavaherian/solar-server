/*
  SDController.hpp - SD Library for Solar Server.
*/
#ifndef SDCONTROLLER_H
#define SDCONTROLLER_H

#include <SPI.h>
#include <SD.h>

class SDController
{
  public:
    SDController();
    void init();
  private:
    const int chipSelect = 0;
  protected:
    Sd2Card card;
    SdVolume volume;
    SdFile root;
};

#endif
