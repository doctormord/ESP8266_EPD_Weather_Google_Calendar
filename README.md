# ESP8266 EPD Weather & Google Calendar

Project Page on [360customs.de](http://www.360customs.de/2018/07/4-2-epd-e-ink-display-esp8266-wetter-google-kalender/)

<a href="http://www.360customs.de/wp-content/uploads/2018/07/IMG_0119.jpg" target="_blank"><img src="http://www.360customs.de/wp-content/uploads/2018/07/IMG_0119.jpg" alt="WF0402T8DCF1300E4 FFC modification" width="480" height="640" border="10" />

### Youtube Demonstration
<a href="http://www.youtube.com/watch?feature=player_embedded&v=1bwpUlR_0Uc" target="_blank"><img src="http://img.youtube.com/vi/1bwpUlR_0Uc/0.jpg" alt="4.2 EPD E-Paper WiFi ESP8266 Weather Station and Google Calendar" width="480" height="360" border="10" /></a>

This project is a mixup of different other sources/projects to provide the following

- ESP8266 based info screen with 
- 4.2" EPD electronic paper display like from [Waveshare](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module),  [Dalian Good Display](http://www.e-paper-display.com) or [Weifeng / Wuxi Vision Peak Technology](http://www.wf-tech.com/en/about.asp?id=1) to
- show actual weather from openweathermap.org (needs API key) and
- show appointments from Google calendar (needs script at Google site)

The initially project is an older Version of the ["ESP32 4.2 ePaper Weather Display"](https://github.com/G6EJD/ESP32-42e-Paper-Weather-Display-) by [David Bird](http://g6ejd.dynu.com). Back in the days this was made to be used with the WeatherUnderground API. As WeatherUnderground dropped its free API access the script needed to be adapted for a different weather provider, so i managed to adapt it for the use with [OpenWeatherMap](https://openweathermap.org).
## Hardware used

- Wemos D1 Mini PCB (or clone) - any ESP8266 will most likely do if enough GPIO are available to connect the Display in 4-Wire-SPI-Mode
- 4.2" EPD which is compatible with the [GxEPD](https://github.com/ZinggJM/GxEPD) library
- FCC adapter to connect the display to the ESP8266 (There are display modules which include the adapter already)
- LiIon battery 3.7V 1000mAh
- LiIon battery charger with protection circuit like the common TP4056 based 5V 1A USB modules

### Compatible Displays
- [Waveshare 4.2" EPD Black/White](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module)
-  Wuxi Vision Peak Technology "WF0402T8DCF1300E4" used in Samsung s-label electronic shelf label (E2X-ST-GM42001) (needs adapter)

## WF0402T8DCF1300E4 specific!!
![#f03c15](https://placehold.it/15/f03c15/000000?text=+) **WARNING**


>When using the **WF0402T8DCF1300E4** (like from Samsung E2X-ST-GM42001 ESL) an connection adapter, which is [available from >Waveshare](https://www.waveshare.com/wiki/E-Paper_Driver_HAT), is needed. To make the Display work, it is needed to salvage >the FFC connector from the Samsung PCB and solder it in place of the original connector on the "E-Paper Driver Hat" because >the **WF0402T8DCF1300E4** has pad connections on the opposite side than the WaveShare displays - pins/contacts facing down, not up. If you just plug in the displays FFC downside up, you'll break the display. Refer to the following picture:
>
> <a href="http://www.360customs.de/wp-content/uploads/2018/07/IMG_0129.jpg" target="_blank"><img src="http://www.360customs.de/wp-content/uploads/2018/07/IMG_0129.jpg" alt="WF0402T8DCF1300E4 FFC modification" width="640" height="640" border="10" />
> 

![#f03c15](https://placehold.it/15/f03c15/000000?text=+) **Not doing so will brick your display**

## Software/Libraries used

- [ESP8266 Arduino](https://github.com/esp8266/Arduino) - 2.4.2
- [GxEPD](https://github.com/ZinggJM/GxEPD) - 2.3.14
- [Adafruit GFX Fonts](https://github.com/adafruit/Adafruit-GFX-Library) - 1.3.6
- [Timezone](https://github.com/JChristensen/Timezone) - 1.2.0
- Time - 1.5.0
- time_ntp.h - NTP & time routines for ESP8266 for ESP8266 adapted Arduino IDE by Stefan Thesen 05/2015
- [ArduinoJSON](https://github.com/bblanchon/ArduinoJson) - 5.13.5
- [HTTPSRedirect](https://github.com/electronicsguy/ESP8266/tree/master/HTTPSRedirect) - 2.0
- [ESP8266 Arduino WiFiManager](https://github.com/tzapu/WiFiManager) - 0.14.0
- Watchdog - 1.2.0
- WebSockets - 2.1.1
- 

- [Google Script Code](https://script.google.com/home)

## Setup
### Hardware
#### Display Connection

Mapping suggestion taked from GxEPD library.

##### Wemos D1 Mini <-> Display SPI

- BUSY -> D2
- RST -> D4
- DC -> D3
- CS -> D8
- CLK -> D5
- DIN -> D7
- GND -> GND
- 3.3V -> 3.3V

##### Generic ESP8266 <-> Display SPI

- BUSY -> GPIO4
- RST -> GPIO2
- DC -> GPIO0
- CS -> GPIO15
- CLK -> GPIO14
- DIN -> GPIO13
- GND -> GND
- 3.3V -> 3.3V

#### Sleep-Mode/HW-Reset

To have the watchdog reseting the ESP8266 a connection from D0 (GPIO16) to RST needed. Use either a resistor [with a value lower than 350ohms or a schottky diode](https://github.com/universam1/iSpindel/issues/59#issuecomment-300329050). A direct connection with resistor/diode renders the code upload/reset functionality useless/non-working.

#### Maximum power saving

To have maximum power saving, the Li-Ion battery (4.2V-3.0V) is connected directly to the 5V line. Using the USB-input always enables the USB-UART-IC (CH340A) which pulls several mA even if not having an open connection. (This is due to some voltage greater than 0.0V on D+/D-) This isn't the case when using either an USB-cable without D+/D- connected or by connecting the battery directly. The latter also saves power by avoiding conversation losses from a battery-powerbank-solution where the battery voltage first is stepped/boosted-up to 5V before getting into the Wemos D1 Mini which then brings down the voltage to 3.3V by an LDO.

The used LDO on the Wemos D1 Mini is a 75mV dropout type, so running down the battery to about 2.9V will still powering the ESP8266 well enough.

Quiscent power consumption is then about 30-35uA while sleeping.

![#f03c15](https://placehold.it/15/f03c15/000000?text=+) **WARNING**

> Always disconnect the battery when using USB-programming to avoid damaging the battery by overvoltage.

### Software
#### Google Script
Copy the provided Google script code into a new project/script at the [Google script console](https://script.google.com/home). Setup the calendar name to your needs. The hit ##Publish -> as WebApp##. Copy the **WebApp-Url** to use in your script. Pasting this adress into your browser should show up your appointments in simple text-form.

You need to run the WebApp as **me** and allow **everyone, even anonymous** to access the script. The code/request is secured by the ScriptKey which is unique and most likely hard to guess. Everytime to apply changes to the Google script, you need to increase the projects revision counter to make it work. Put the ScriptKey into the .INO project file.

#### OpenWeatherMap
Register an account at [OpenWeatherMap](https://openweathermap.org/appid) to get an API key and paste it into the .INO project file.
