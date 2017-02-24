/*
  Config.cpp - Library for client config.
*/
#include "Config.hpp"

Config::Config(String path)
{
  if(!path.startsWith("/")) path = "/" + path;
  _configPath = path;
}

bool Config::exists()
{
  bool exists = SPIFFS.exists(_configPath);
  Serial.print(_configPath);
  Serial.print(" config exists ");
  Serial.println(exists);
  return exists;
}

void Config::readConfigSettings()
{

  char character;
  String settingName;
  String settingValue;
  File myFile = SPIFFS.open(_configPath, "r");

  if (myFile) {

    Serial.println("opened config file for reading " +  _configPath);

    while (myFile.available()) {

      character = myFile.read();
      while((myFile.available()) && (character != '[')){
        character = myFile.read();
      }

      character = myFile.read();
      while((myFile.available()) && (character != '=')){
        settingName = settingName + character;
        character = myFile.read();
      }

      character = myFile.read();
      while((myFile.available()) && (character != ']')){
        settingValue = settingValue + character;
        character = myFile.read();
      }

      if (character == ']') {

        //Debug
        Serial.print("Name:");
        Serial.println(settingName);
        Serial.print("Value :");
        Serial.println(settingValue);

        // Apply the value to the parameter
        applyConfigSetting(settingName, settingValue);

        // Reset Strings
        settingName = "";
        settingValue = "";
      }
    }
    // close the file:
    myFile.close();

  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening " +  _configPath);
  }

}

void Config::applyConfigSetting(String settingName, String settingValue)
{
   if(settingName == "SSID") {
     _ssid = settingValue;
   }
   if(settingName == "PASSWORD") {
     _password = settingValue;
   }
}

bool Config::writeConfigSettings()
{
  
}

const String& Config::ssid()
{
  return _ssid;
}

const String& Config::password()
{
  return _password;
}
