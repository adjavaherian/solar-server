/*
  Config.hpp - Library for Client config.
  http://overskill.alexshu.com/saving-loading-settings-on-sd-card-with-arduino/
*/
#ifndef CONFIG_H
#define CONFIG_H

#define FS_NO_GLOBALS

#include <FS.h>
#include "Arduino.h"

class Config
{
  public:
    Config();
    Config(String path);
    bool exists();
    void readConfigSettings();
    void applyConfigSetting(String settingName, String settingValue);
    const String& ssid();
    const String& password();
    const String& host();
    bool writeConfigSettings();
    void removeConfigFile();
    void resetConfigSettings();
  private:
    String _configPath;
    String _ssid;
    String _password;
    String _host;
};

#endif
