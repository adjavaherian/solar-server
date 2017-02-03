# solar-server
Solar powered webserver on ESP8266.  This project is based on the [Arduino core for ESP8266](https://github.com/esp8266/Arduino)

## Requirements
* Arduino 1.6.4 or higher
* [Arduino core for ESP8266](https://github.com/esp8266/Arduino)

## Install Board Manager
* Start Arduino and open Preferences window.
Enter `http://arduino.esp8266.com/stable/package_esp8266com_index.json` into Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.
* Open Boards Manager from Tools > Board menu and install esp8266 platform (and don't forget to select your ESP8266 board from Tools > Board menu after installation).

## Flash your ESP8266
* If you are using a Gizwitts serial breakout board you should see a reset button and a flash button on the breakout board.
* Open up the `SolarServer` sketch in Arduino
* Make sure the Arduino Serial Monitor is closed
* While holding the reset button, plug the ESP8266 and USB cable into your computer to enable board uploads.
  * Your board is now flashable, but you may not see a light indicating that.
* In Arduino IDE, Select Tools > Board > NodeMCU 1.0 (ESP-12E) or your specific board, to select your board baud rate
* Make sure your serial / usb port is also set Tools > Port > /dev/usb, etc.
* Edit the SSID, Password and host in the sketch
  * `const char* ssid = "yourssid";`
  * `const char* password = "yourpassword";`
  * `const char* host = "solar-server";`
* Compile and upload the server sketch
  * Arduino console should look like this:
    * `Sketch uses 279331 bytes (26%) of program storage space. Maximum is 1044464 bytes.
Global variables use 36780 bytes (44%) of dynamic memory, leaving 45140 bytes for local variables. Maximum is 81920 bytes.
Uploading 283472 bytes from /var/folders/z5/jlv1d8yn2lzcjjh2w9z3lrjc0000gn/T/arduino_build_20395/SolarServer.ino.bin to flash at 0x00000000
................................................................................ [ 28% ]
................................................................................ [ 57% ]
................................................................................ [ 86% ]
.....................................                                            [ 100% ]`
* When complete, enable your serial monitor and reset the ESP module with reset button or power toggle
  * Your serial monitor should look like this at the bottom
    * `Connected! IP address: 192.168.0.106
Open http://solar-server.local/edit to see the file browser
solar-server started
`
* Visit the solar server in your browser
  [http://solar-server.local](http://solar-server.local)

## Etc.
* [ESP Tools](https://github.com/igrr/esptool-ck)
* [SPI Filesystem](https://github.com/pellepl/spiffs)
* [ESP8266 Wikipedia](https://en.wikipedia.org/wiki/ESP8266)
* [ESP8266 Tutorial by Vimal](https://vimalb.github.io/IoT-ESP8266-Starter/Witty/info.html)
* [CH340 Serial Driver MacOS Sierra](https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver)
* [ESP8266 Community Forum](http://www.esp8266.com/viewforum.php?f=25)
