/*
  Config.hpp - Library for Client config.
  http://overskill.alexshu.com/saving-loading-settings-on-sd-card-with-arduino/
*/
#ifndef CONFIG_H
#define CONFIG_H

#include <FS.h>
#include "Arduino.h"

class Config
{
  public:
    Config(String path);
    Config();
    bool exists();
    void readConfigSettings();
    void applyConfigSetting(String settingName, String settingValue);
    const String& ssid();
    const String& password();
    bool writeConfigSettings();
    void removeConfigFile();
    void resetConfigSettings();
    const char* configFile = "config.txt";
  private:
    String _configPath;
    String _ssid;
    String _password;
};

#endif
