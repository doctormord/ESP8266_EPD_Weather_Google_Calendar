# ESP8266 EPD Weather & Google Calendar

<a href="http://www.youtube.com/watch?feature=player_embedded&v=1bwpUlR_0Uc" target="_blank"><img src="http://img.youtube.com/vi/1bwpUlR_0Uc/0.jpg" alt="4.2 EPD E-Paper WiFi ESP8266 Weather Station and Google Calendar" width="480" height="360" border="10" /></a>

This project is a mixup of different other sources to provide the following

- ESP8266 based info screen with 
- 4.2" EPD electronic paper display like from [https://www.waveshare.com/wiki/4.2inch_e-Paper_Module](Waveshare) to
- showing actual weather from openweathermap.org (needs API key) and
- showing appointments from Google calendar (needs script at Google site)

## Hardware used

- Wemos D1 Mini PCB (or clone) - any ESP8266 will most likely do if enough GPIO are available to connect the Display in 4-Wire-SPI-Mode
- 4.2" EPD which is compatible with the [GxEPD](https://github.com/ZinggJM/GxEPD) library
- FCC adapter to connect the display to the ESP8266 (There are display modules which include the adapter already)
- LiIon battery 3.7V 1000mAh
- LiIon battery charger with protection circuit like the common TP4056 based 5V 1A USB modules

### Compatible Displays
- [Waveshare 4.2" EPD Black/White](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module)
-  Wuxi Vision Peak Technology "WF0402T8DCF1300E4" used in Samsung s-label electronic shelf label (E2X-ST-GM42001) (needs adapter)

#### WF0402T8DCF1300E4 specific!!

>>>
**WARNING**

When using the **WF0402T8DCF1300E4** (like from Samsung E2X-ST-GM42001 ESL) an connection adapter, which is [available from Waveshare](https://www.waveshare.com/wiki/E-Paper_Driver_HAT), is needed. To make the Display work, it is needed to salvage the FFC connector from the Samsung PCB and solder it in place of the original connector on the "E-Paper Driver Hat" because the **WF0402T8DCF1300E4** has pad connections on the opposite side than the WaveShare displays - so pinout order is in reverse.

**Not doing so might brick your display**
>>>

## Software/Libraries used

- [ESP8266 Arduino](https://github.com/esp8266/Arduino)
- [GxEPD](https://github.com/ZinggJM/GxEPD)
- [Adafruit GFX Fonts](https://github.com/adafruit/Adafruit-GFX-Library)
- [Timezone](https://github.com/JChristensen/Timezone)
- time_ntp.h - NTP & time routines for ESP8266 for ESP8266 adapted Arduino IDE by Stefan Thesen 05/2015
- [ArduinoJSON](https://github.com/bblanchon/ArduinoJson)
- [HTTPSRedirect](https://github.com/electronicsguy/ESP8266/tree/master/HTTPSRedirect)
- [ESP8266 Arduino WiFiManager](https://github.com/tzapu/WiFiManager)

- [Google Script Code](https://script.google.com/home)

## Setup
### Hardware
#### Display Connection

### Software
#### Google Script
Copy the provided Google script code into a new project/script at the [Google script console](https://script.google.com/home). Setup the calendar name to your needs. The hit ##Publish -> as WebApp##. Copy the **WebApp-Url** to use in your script. Pasting this adress into your browser should show up your appointments in simple text-form.

You need to run the WebApp as **me** and allow **everyone, even anonymous** to access the script. The code/request is secured by the ScriptKey which is unique and most likely hard to guess. Everytime to apply changes to the Google script, you need to increase the projects revision counter to make it work. Put the ScriptKey into the .INO project file.

#### OpenWeatherMap
Register an account at [OpenWeatherMap](https://openweathermap.org/appid) to get an API key and paste it into the .INO project file.
