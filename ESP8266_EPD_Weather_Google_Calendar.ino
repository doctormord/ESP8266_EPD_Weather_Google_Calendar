/*########################   Weather Display  #############################
   Receives and displays the weather forecast from the OpenWeatherMap and then displays using a JSON decoder.
   Weather data received via WiFi connection to OpenWeatherMap Servers and using their 'Forecast' API and data
   is decoded using Copyright Benoit Blanchon's (c) 2014-2017 excellent JSON library.
   This source code is protected under the terms of the MIT License and is copyright (c) 2017 by
   David Bird,
   Christian Weidner
   and permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
   documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use,
   copy, modify, merge, publish, distribute, but not to sub-license and/or to sell copies of the Software or to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

     See more at http://dsbird.org.uk */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ArduinoJson.h>     // https://github.com/bblanchon/ArduinoJson
#include "HTTPSRedirect.h"  // https://github.com/electronicsguy/ESP8266/tree/master/HTTPSRedirect



//#include "DebugMacros.h"

#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h> // https://github.com/JChristensen/Timezone
#include "time_ntp.h"  // NTP & time routines for ESP8266 for ESP8266 adapted Arduino IDE by Stefan Thesen 05/2015 - free for anyone

#include <SPI.h>

// GxEPD library - https://github.com/ZinggJM/GxEPD
#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.cpp>
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

// Adafruit GFX
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <pgmspace.h>


// Params needed to fetch events from Google Calendar
char const * const dstHost = "script.google.com";
char const * const dstPath = "/macros/s/yourkeyV31W-uV31XPBgCVS9qmjskw/exec"; // script path including key
int const dstPort = 443;
int32_t const timeout = 2000;
int const GoogleServerMaxRetry = 1; //maximum tries to reach google server.

// define buffersize for several buffers - needs to be tidy, otherwise HTTPS crashs due to low free heap
int const bufferSize = 1350;

// #define DEBUG
#ifdef DEBUG
#define DPRINT(...)    Serial.print(__VA_ARGS__)
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#endif

// for time conversion
// Central European Time (Frankfurt, Paris)

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

// ntp flag
bool bNTPStarted = false;



// 24 x 24 gridicons_info_outline
const unsigned char gridicons_info_outline[] PROGMEM = { /* 0X01,0X01,0XB4,0X00,0X40,0X00, */
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x81,
  0xFF, 0xFE, 0x00, 0x7F, 0xF8, 0x3C, 0x1F, 0xF1,
  0xFF, 0x0F, 0xF3, 0xFF, 0xCF, 0xE3, 0xE7, 0xC7,
  0xE7, 0xE7, 0xE7, 0xCF, 0xFF, 0xF3, 0xCF, 0xFF,
  0xF3, 0xCF, 0xE7, 0xF3, 0xCF, 0xE7, 0xF3, 0xCF,
  0xE7, 0xF3, 0xC7, 0xE7, 0xE3, 0xE7, 0xE7, 0xE7,
  0xE7, 0xE7, 0xE7, 0xF3, 0xFF, 0xCF, 0xF1, 0xFF,
  0x0F, 0xF8, 0x7E, 0x1F, 0xFE, 0x00, 0x7F, 0xFF,
  0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


// pins_arduino.h, e.g. WEMOS D1 Mini
//static const uint8_t SS    = D8;
//static const uint8_t MOSI  = D7;
//static const uint8_t MISO  = ;
//static const uint8_t SCK   = D5;
// GxIO_SPI(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst = -1, int8_t bl = -1);
GxIO_Class io(SPI, D8, D3, D4);
// GxGDEP015OC1(GxIO& io, uint8_t rst = D4, uint8_t busy = D2);
GxEPD_Class display(io, D4, D6);


//------ NETWORK VARIABLES---------
// Use your own API key by signing up for a free developer account athttps://openweathermap.org/api
char const * const API_key       = "yourapikey57f8a1351f6f45f9b242b60f5";            // See: https://openweathermap.org/api (change here with your KEY)
char const * const City          = "Moabit";                    // Your home city
char const * const Country       = "de";                          // Your country
char const * const LangCode      = "en";                          // Language Code for localization
char const * const DataMode      = "json";                        // Data mode for response
char const * const Units         = "metric";                      // Units for numbers -> metric, imperial
char const * const RowCount      = "3";                           // JSON data set count, like list entries in forecast

char const * const owmserver = "api.openweathermap.org";        // Address for OpenWeatherMap
// unsigned long        lastConnectionTime = 0;          // Last time you connected to the server, in milliseconds

//const unsigned long  postingInterval    = 15L * 1000L; // Delay between updates, in milliseconds, WU allows 500 requests per-day maximum, set to every 10-mins or 144/day
unsigned long updateInterval = 1L * 60L * 1000000L; //Delay between updates, in microseconds for deepsleep - this relates to 1 minutes

int ConnectionTimedOut = 0;

//################ PROGRAM VARIABLES and OBJECTS ################
// Conditions
String  icon0, high0, low0, temp0, pressure0, humidity0, datetime0,
        icon1, high1, low1, temp1, pressure1, humidity1, datetime1;

// Actual
String  sunrise, sunset, conditionsa, icona, temp_lowa, temp_higha, tempa, pressurea, humiditya, datetimea;

// Google calendar
String appointment0, appointment1, appointment2, appointment3;
String appdate0, appdate1, appdate2, appdate3;
String appDateString0, appDateString1, appDateString2, appDateString3;

// Helper string to get proper phrase from weather-icon-id
String conditionPhrase;
String localTimeString;

char const * const weekDay0 = "Montag";
char const * const weekDay1 = "Dienstag";
char const * const weekDay2 = "Mittwoch";
char const * const weekDay3 = "Donnerstag";
char const * const weekDay4 = "Freitag";
char const * const weekDay5 = "Samstag";
char const * const weekDay6 = "Sonntag";
/*
  const char* ssid     = "Mausi";
  const char* password = "Emilia01";
  const char* host     = "api.openweathermap.org";
*/

WiFiClient client; // wifi client object

// #include "imglib/gridicons_align_right.h"

void configModeCallback (WiFiManager *myWiFiManager) {
  DPRINTLN(F("failed to connect and hit timeout"));
  DPRINT(F("Entered config mode at: ")); DPRINT(myWiFiManager->getConfigPortalSSID()); DPRINT(F(" IP: ")); DPRINTLN(WiFi.softAPIP());
  display.fillScreen(GxEPD_WHITE);
  display.drawBitmap(gridicons_info_outline, 138, 188, 24, 24, GxEPD_BLACK, 1);
  display.fillRect(0, 380, 300, 20, GxEPD_BLACK); // Box für footer
  display.setTextColor(GxEPD_WHITE);
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(4, 395);
  display.println("SSID: " + myWiFiManager->getConfigPortalSSID() + "  IP:");
  display.setCursor(180, 395);
  display.println(WiFi.softAPIP());
  display.update();

  ConnectionTimedOut = 1;
}

void setup() {

 
  unsigned long startTime = millis();

  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(30);
  wifiManager.setBreakAfterConfig(true);
  wifiManager.setAPCallback(configModeCallback);

  // set callback that gets called when connecting to previous WiFi fails, and
  // enters Access Point mode


  Serial.begin(115200);
  DPRINT(F("Setup time:")); DPRINTLN(startTime);
  display.init();
  display.setRotation(1);
  //display.fillScreen(GxEPD_WHITE);
  display.fillRect(0, 380, 300, 20, GxEPD_BLACK); // Box für footer
  display.setTextColor(GxEPD_WHITE);
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(4, 395); display.println("Refreshing..");
  display.updateWindow(0, 0, 20, 300, false); // fast update footer

  // StartWiFi(ssid, password);

  // do not try to connect endlessly to save power
  
  wifiManager.setConnectTimeout(5);
  wifiManager.autoConnect("SmartEPD");

  //if you get here you have connected to the WiFi

  DPRINTLN(F("connected...yeey :)"));

  display.fillRect(0, 380, 300, 20, GxEPD_BLACK); // Box für footer
  display.setTextColor(GxEPD_WHITE);
  display.setCursor(4, 395); display.println("Refreshing..");
  display.setCursor(110, 395); display.println(WiFi.localIP());

  display.updateWindow(0, 0, 20, 300, false); //fast update footer


  ////////////////////////////////////////////
  // below here we got a valid WIFI connection
  ////////////////////////////////////////////

  // connect to NTP and get time each day
  // timelib now synced up to UTC

  DPRINTLN(F("Setup sync with NTP service."));
  setSyncProvider(getNTP_UTCTime1970);

  // UTC
  time_t tT = now();
  DPRINT("UTC: "); DPRINTLN(ctime(&tT));

  // local time
  time_t tTlocal = CE.toLocal(tT);
  DPRINT(F("Local Time: ")); DPRINTLN(ctime(&tTlocal));

  localTimeString = weekday(tTlocal);

  if (localTimeString == "2") localTimeString = weekDay0;
  else if (localTimeString == "3") localTimeString = weekDay1;
  else if (localTimeString == "4") localTimeString = weekDay2;
  else if (localTimeString == "5") localTimeString = weekDay3;
  else if (localTimeString == "6") localTimeString = weekDay4;
  else if (localTimeString == "7") localTimeString = weekDay5;
  else if (localTimeString == "1") localTimeString = weekDay6;
  else localTimeString = " ";


  localTimeString += F(" - "); localTimeString += day(tTlocal) / 10; localTimeString += day(tTlocal) % 10; localTimeString += F("."); localTimeString += month(tTlocal) / 10; localTimeString += month(tTlocal) % 10; localTimeString += F("."); localTimeString += year(tTlocal);
  localTimeString += F(" - "); localTimeString += hour(tTlocal) / 10 ; localTimeString += hour(tTlocal) % 10; localTimeString += F(":"); localTimeString += minute(tTlocal) / 10; localTimeString += minute(tTlocal) % 10;

  DPRINT(F("localTimeString: ")); DPRINT(localTimeString);

  unsigned long currentTime = millis();
  unsigned long elapsedTime = (currentTime - startTime);
  //startTime = currentTime;
  Serial.print(F("Time since start until weather start: ")); Serial.print(elapsedTime); Serial.println(F(" milliseconds"));
  elapsedTime = currentTime;

  //lastConnectionTime = millis();
  obtain_forecast("forecast");
  obtain_forecast("weather");

  currentTime = millis();
  unsigned long weatherTime = (currentTime - elapsedTime);
  //startTime = currentTime;
  Serial.print(F("Time to get weather done: ")); Serial.print(weatherTime); Serial.println(F(" milliseconds"));
  elapsedTime = currentTime;

  syncCalendar();
  DPRINTLN(F("Fetching data is done now."));

  currentTime = millis();
  unsigned long googleTime = (currentTime - elapsedTime);
  Serial.print(F("Time used for google calendar sync: ")); Serial.print(googleTime); Serial.println(F(" milliseconds"));

  WiFi.forceSleepBegin(); // power down WiFi, as it is not needed anymore.

  DPRINTLN(F("Powering down WiFi now!"));
  currentTime = millis();
  elapsedTime = (currentTime - startTime);
  startTime = currentTime;
  //DPRINT(F("Time since start in high power mode: "));DPRINT(elapsedTime);DPRINTLN(F(" milliseconds"));
  Serial.print(F("Time since start in high power mode: ")); Serial.print(elapsedTime); Serial.println(F(" milliseconds"));

  // bat check

  // 0.005567 times RAW digit = volt for given resistor values (470k/100k)
  // treshholds for low bat warnings
  // 3.0V = empty = 539
  // 3.2V = please charge = 593
  // 3.4V = low bat = 611

  // voltage at A0

  int batRAW = getADCRAW();
  float batVolt = batRAW * 0.005567;
  String batStatus;

  if      (batRAW <= 539) batStatus = F("Batterie leer!"); // totally flat
  else if (batRAW <= 593) batStatus = F("Jetzt laden!"); // please charge
  else if (batRAW <= 611) batStatus = F("Bald laden!"); // first warning for low bat
  else if (batRAW >  782)  batStatus = F("USB Connect!"); // must be USB or Powerbank
  //  else                    batStatus = F("Batterie okay.");
  else                    batStatus = (batVolt);

  display.setFont(&FreeSans9pt7b); display.setTextColor(GxEPD_BLACK);
  display.setCursor(4, 14); display.println(batStatus);

  //last check for adjusting the deep sleep intervall

  int localHour = hour(tTlocal);

  DPRINT(F("localHour: ")); DPRINTLN(localHour);
  
 
  if (localHour >= 6 && localHour <= 9) updateInterval = 15L * 60L * 1000000L; // 15 minutes in the morning from 06:00 till 09:59
  else if (localHour >= 18 && localHour <= 20) updateInterval = 15L * 60L * 1000000L; // 15 minutes in the evening from 18:00 till 20:59
  // else if (localHour > 0 || localHour <= 5) updateInterval = 60U * 60L * 1000000L; // 60 minutes updates at night between 01:00 and 04:59
  else updateInterval = 60U * 60L * 1000000L; // 60 minutes for the rest of the day


  // updateInterval = 1U * 60L * 1000000L; // 1 minute refresh for debug

  DisplayData(); // finally update display

  unsigned long updateMinutes = updateInterval / (1000000L * 60L);
  Serial.print(F("Next update in: ")); Serial.print(updateMinutes);  Serial.println(F(" minutes."));

  // Get display into ULP mode
  int rstDelay = 5000;
  DPRINT(F("Pulling RST low in ")); DPRINT(rstDelay); DPRINTLN(F("us.."));
  delay(rstDelay);
  DPRINTLN(F("Pulling RST low now!"));
  pinMode(2, OUTPUT); // RST
  digitalWrite(2, LOW);
  delay(50);
  // pinMode(2, INPUT); // back to input to save power
  digitalWrite(2, HIGH); // force RST high again to save power - Wemos has 10/12k ext. pullup here.

  //Good Bye

  display.powerDown(); // still needed?

  currentTime = millis();
  elapsedTime = (currentTime - startTime);
  DPRINT(F("Time since start in low power mode: ")); DPRINT(elapsedTime); DPRINTLN(F(" milliseconds"));
  Serial.print(F("Time since start in low power mode: ")); Serial.print(elapsedTime); Serial.println(F(" milliseconds"));

  // ESP.deepSleep(updateInterval, WAKE_RF_DEFAULT); // DeepSleep then restart - Connect RST to GPIO16
  ESP.deepSleep(updateInterval, WAKE_NO_RFCAL); // provides faster reconnect after deep sleep but is actually broken

}


void loop() {

}

void DisplayData() { // Display is 400x300 resolution


  // get condition phrase from icon status
  conditionPhrase = getConditionPhrase(icona);
  DPRINTLN("icona: " + conditionPhrase);

  // Do some basic layout first

  // Breite links 120
  // Höhe oben 160

  //display.drawLine(0,60,300,60,GxEPD_BLACK);
  display.drawFastHLine(0, 160, 300, GxEPD_BLACK); // Trenner Wetter Kalender
  display.drawFastVLine(120, 0, 160, GxEPD_BLACK); // Trenner Wetter links / Wettervorschau rechts
  display.drawFastHLine(0, 140, 120, GxEPD_BLACK); // Trenner Wetter links für untere Felder
  display.drawFastVLine(44, 140, 20, GxEPD_BLACK); // Trenner Wetter links untere 2 Felder

  display.drawFastHLine(128, 80, 164, GxEPD_BLACK); // Trenner Wetter rechts Vorhersage oben unten

  display.fillRect(0, 80, 120, 60, GxEPD_BLACK); // Box für aktuelle Temperatur

  display.fillRect(0, 380, 300, 20, GxEPD_BLACK); // Box für footer

  display.drawFastHLine(30, 214, 240, GxEPD_BLACK); // Calendar inner divider0
  display.drawFastHLine(30, 269, 240, GxEPD_BLACK); // Calendar inner divider1
  display.drawFastHLine(30, 324, 240, GxEPD_BLACK); // Calendar inner divider2

  DisplayConditionIcon(60, 44, icona, 10); // Display icon for actual condition
  DisplayConditionIcon(148, 22, icon0, 5); // Display icon for forecast0
  DisplayConditionIcon(148, 102, icon1, 5); // Display icon for forecast1

  display.setFont(&FreeSans9pt7b);
  display.setTextColor(GxEPD_BLACK);

  // Forecast0 is 3h ahead
  //
  // get condition phrase from icon status
  conditionPhrase = getConditionPhrase(icon0);
  display.setCursor(128, 56);  display.println(conditionPhrase);
  display.setCursor(128, 76);  display.println(humidity0 + "% / " + getValue(pressure0, '.', 0) + "hPa");
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(240, 34);  display.print(getValue(temp0, '.', 0));
  display.setFont(&FreeSans9pt7b);

  // Forecast0 is 6h ahead
  //
  // get condition phrase from icon status
  conditionPhrase = getConditionPhrase(icon1);
  display.setCursor(128, 136);  display.println(conditionPhrase);
  display.setCursor(129, 156);  display.println(humidity1 + "% / " + getValue(pressure1, '.', 0) + "hPa");
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(240, 114);  display.print(getValue(temp1, '.', 0));
  display.setFont(&FreeSans9pt7b);

  // Actual weather is in time
  //
  display.setFont(&FreeSansBold24pt7b);
  display.setTextColor(GxEPD_WHITE);
  display.setCursor(46, 134);
  display.print(getValue(tempa, '.', 0));
  display.setFont(&FreeSans9pt7b);

  // get condition phrase from icon status
  conditionPhrase = getConditionPhrase(icona);
  display.setCursor(2, 94);  display.println(conditionPhrase);
  display.setTextColor(GxEPD_BLACK);

  //display.setCursor(40, 75);  display.println("Hoch/Tief: " + temp_higha + "/" + temp_lowa + " \xB0""C");
  display.setCursor(4, 156);  display.println(humiditya + "%");
  display.setCursor(46, 156);  display.println(pressurea + "hPa");

  // Update timestamp from forecast
  // includes daytime from forecast
  // display.setCursor(130, 20); display.print(getValue(datetime0, ' ', 3));
  // display.setCursor(130, 40); display.print(getValue(datetime1, ' ', 3));


  // Now for the calendar
  //
  // datestrings for appointments
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(4, 182);  display.println(appDateString0);
  display.setCursor(4, 237);  display.println(appDateString1);
  display.setCursor(4, 292);  display.println(appDateString2);
  display.setCursor(4, 347);  display.println(appDateString3);

  display.setFont(&FreeSans9pt7b);

  // list of appointments, empty list entries wont be shown
  display.setCursor(4, 202);  display.println(appointment0.substring(0, 32));
  display.setCursor(4, 257);  display.println(appointment1.substring(0, 32));
  display.setCursor(4, 312);  display.println(appointment2.substring(0, 32));
  display.setCursor(4, 367);  display.println(appointment3.substring(0, 32));

  // check if any appointment is marked as high priority by an "!" somewhere in title
  DPRINT(F("appointment0 index : "));
  DPRINTLN(appointment0.indexOf("!"));


  if (appointment0.indexOf("!") >= 0) display.drawBitmap(gridicons_info_outline, 271, 164, 24, 24, GxEPD_BLACK, 1);
  if (appointment1.indexOf("!") >= 0) display.drawBitmap(gridicons_info_outline, 271, 219, 24, 24, GxEPD_BLACK, 1);
  if (appointment2.indexOf("!") >= 0) display.drawBitmap(gridicons_info_outline, 271, 274, 24, 24, GxEPD_BLACK, 1);
  if (appointment3.indexOf("!") >= 0) display.drawBitmap(gridicons_info_outline, 271, 329, 24, 24, GxEPD_BLACK, 1);

  // add local time to footer
  display.setFont(&FreeSansBold9pt7b);
  display.setTextColor(GxEPD_WHITE);
  display.setCursor(4, 395);  display.println(localTimeString);

  display.update(); // final panel update before deepsleep


}

//
// Localize condition strings based on OpenWeatherMap icon-id
//

String getConditionPhrase(String iconstatus) {

  String phrase;
  if      (iconstatus == "10d" || iconstatus == "10n") phrase = "eventl. Regen"; // expect rain
  else if (iconstatus == "09d" || iconstatus == "09n") phrase = "Regen"; // rain
  else if (iconstatus == "13d" || iconstatus == "13n") phrase = "Schnee"; // snow
  else if (iconstatus == "01d"                       ) phrase = "Klarer Himmel"; // sunny
  else if (iconstatus == "01n"                       ) phrase = "Klare Nacht"; // moon
  else if (iconstatus == "02d"                       ) phrase = "teilw. sonnig"; // partly sunny
  else if (iconstatus == "02n"                       ) phrase = "meist klar"; // partly moon
  else if (iconstatus == "03d"                       ) phrase = "meist wolkig"; // mostly cloudy
  else if (iconstatus == "03n"                       ) phrase = "meist wolkig"; // mostly claudy moon
  else if (iconstatus == "04d" || iconstatus == "04n") phrase = "Bedeckt"; // cloudy
  else if (iconstatus == "11d" || iconstatus == "11n") phrase = "Gewitter"; // thunderstorms
  else if (iconstatus == "50d" || iconstatus == "50n") phrase = "Nebel"; // fog
  else phrase = "";

  return phrase;
}

// Map icon drawing function based on OpenWeatherMap icon-id
//

void DisplayConditionIcon(int x, int y, String IconName, int scale) {

  DPRINTLN(IconName);
  if      (IconName == "10d" || IconName == "10n") ExpectRain(x, y, scale);
  else if (IconName == "09d" || IconName == "09n") Rain(x, y, scale);
  else if (IconName == "13d" || IconName == "13n") Snow(x, y, scale);
  else if (IconName == "01d"                     ) Sunny(x, y, scale);
  else if (IconName == "01n"                     ) Moon(x, y, scale);
  else if (IconName == "02d"                     ) MostlySunny(x, y, scale);
  else if (IconName == "02n"                     ) MostlyMoon(x, y, scale);
  else if (IconName == "03d"                     ) MostlyCloudy(x, y, scale);
  else if (IconName == "03n"                     ) MostlyCloudyMoon(x, y, scale);
  else if (IconName == "04d" || IconName == "04n") Cloudy(x, y, scale);
  else if (IconName == "11d" || IconName == "11n") Tstorms(x, y, scale);
  else if (IconName == "50d" || IconName == "50n") Fog(x, y, scale);
  else Nodata(x, y, scale);
}

void obtain_forecast (String forecast_type) {

  char RxBuf[bufferSize]; // made local char, rather than static
  String request;

  request  = "GET /data/2.5/" + forecast_type + "?q=" + City + "," + Country + "&APPID=" + API_key + "&mode=" + DataMode + "&units=" + Units + "&lang=" + LangCode + "&cnt=" + RowCount + " HTTP/1.1\r\n";
  request += F("User-Agent: Weather Webserver");
  request += F("\r\n");
  request += F("Accept: */*\r\n");
  request += "Host: " + String(owmserver) + "\r\n";
  request += F("Connection: close\r\n");
  request += F("\r\n");
  DPRINTLN(request);
  DPRINT(F("Connecting to ")); DPRINTLN(owmserver);
  WiFiClient httpclient;
  //WiFi.status(); // ?
  if (!httpclient.connect(owmserver, 80)) {
    Serial.println(F("connection failed initially"));
    httpclient.flush();
    httpclient.stop();

    return;
  }
  DPRINT(request);
  httpclient.print(request); //send the http request to the server
  // Collect http response headers and content from Weather Underground, discarding HTTP headers, the content is JSON formatted and will be returned in RxBuf.
  uint16_t respLen = 0;
  bool     skip_headers = true;
  String   rx_line;
  while (httpclient.connected() || httpclient.available()) {
    if (skip_headers) {
      rx_line = httpclient.readStringUntil('\n');
      DPRINTLN(rx_line);
      if (rx_line.length() <= 1) { // a blank line denotes end of headers
        skip_headers = false;
      }

    }
    else {
      int bytesIn;
      bytesIn = httpclient.read((uint8_t *)&RxBuf[respLen], sizeof(RxBuf) - respLen);
      //Serial.print(F("bytesIn ")); Serial.println(bytesIn);
      if (bytesIn > 0) {
        respLen += bytesIn;
        if (respLen > sizeof(RxBuf)) respLen = sizeof(RxBuf);
      }
      else if (bytesIn < 0) {
        DPRINT(F("read error "));
        DPRINTLN(bytesIn);
      }
    }
    delay(1);
  }
  httpclient.flush();
  httpclient.stop();

  if (respLen >= sizeof(RxBuf)) {
    Serial.print(F("RxBuf overflow "));
    Serial.println(respLen);
    delay(1000);
    return;
  }

  RxBuf[respLen++] = '\0'; // Terminate the C string
  DPRINT(F("respLen ")); DPRINTLN(respLen); DPRINTLN(RxBuf);
  if (forecast_type == "forecast") {
    compileWeather_forecast(RxBuf);
  }
  if (forecast_type == "weather") {
    compileWeather_actual(RxBuf);
  }
}

bool compileWeather_actual(char *json) {

  StaticJsonBuffer<1 * bufferSize> jsonBuffer;

  char *jsonstart = strchr(json, '{'); // Skip characters until first '{' found
  DPRINT(F("jsonstart ")); DPRINTLN(jsonstart);
  if (jsonstart == NULL) {
    Serial.println(F("JSON data missing"));
    return false;
  }
  json = jsonstart;
  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println(F("jsonBuffer.parseObject() failed"));
    return false;
  }
  // Extract weather info from parsed JSON

  tempa = (const char*)root["main"]["temp"];
  temp_lowa = (const char*)root["main"]["temp_min"];
  temp_higha = (const char*)root["main"]["temp_max"];
  icona = (const char*)root["weather"][0]["icon"];
  conditionsa = (const char*)root["weather"][0]["description"];
  pressurea = (const char*)root["main"]["pressure"];
  humiditya = (const char*)root["main"]["humidity"];

/*
  time_t srtime = root["sys"]["sunrise"];
  time_t sstime = root["sys"]["sunset"];

  time_t forecasttime = root["dt"];
  datetimea = ctime(&forecasttime);

  DPRINT(F("Temp actual: ")); DPRINT(tempa); DPRINT(F(" Icon: ")); DPRINTLN(icona);
  DPRINT(F("Sunrise: ")); Serial.println(ctime(&srtime));
  DPRINT(F("Sunset: ")); Serial.println(ctime(&sstime));
*/

  jsonBuffer.clear();
  return true;
}

bool compileWeather_forecast(char *json) {
  DynamicJsonBuffer jsonBuffer(bufferSize);
  // StaticJsonBuffer<1 * 1024> jsonBuffer; // free up some heap

  char *jsonstart = strchr(json, '{');
  DPRINT(F("jsonstart ")); DPRINTLN(jsonstart);
  if (jsonstart == NULL) {
    Serial.println(F("JSON data missing"));
    return false;
  }
  json = jsonstart;

  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println(F("jsonBuffer.parseObject() failed"));
    return false;
  }

  JsonArray& list = root["list"];
  JsonObject& threehourf = list[1];
  JsonObject& sixhourf = list[2];

  // [0] is last forecast close to actual time, [1] is +3h, [2] is +6h
  // to get weather for +6h and +12h, we need to look into [2] and [4]


  icon0  = (const char*)threehourf["weather"][0]["icon"];
  temp0 = (const char*)threehourf["main"]["temp"];
  humidity0 = (const char*)threehourf["main"]["humidity"];
  pressure0 = (const char*)threehourf["main"]["pressure"];

  // get forecast timestamps

  time_t forecasttime = threehourf["dt"];
  datetime0 = ctime(&forecasttime);

  DPRINTLN(F("3h forecast: ")); DPRINTLN(icon0 + " " + high0 + " " + low0 + " " + temp0 + " " + humidity0 + " " + pressure0 + " " + datetime0);

  icon1  = (const char*)sixhourf["weather"][0]["icon"];
  temp1 = (const char*)sixhourf["main"]["temp"];
  humidity1 = (const char*)sixhourf["main"]["humidity"];
  pressure1 = (const char*)sixhourf["main"]["pressure"];

  // get forecast timestamps
  forecasttime = sixhourf["dt"];
  datetime1 = ctime(&forecasttime);

  DPRINTLN(F("6h forecast: ")); DPRINTLN(icon1 + " " + high1 + " " + low1 + " " + temp1 + " " + humidity1 + " " + pressure1 + " " + datetime1);

  jsonBuffer.clear();
  return true;
}


void syncCalendar() {



  // Fetch Google Calendar events for 1 week ahead

  DPRINT(F("Free heap at sync start .. ")); DPRINTLN(ESP.getFreeHeap());

  HTTPSRedirect* client = nullptr; // Setup a new HTTPS client

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(dstPort);
  client->setPrintResponseBody(false);
  client->setContentTypeHeader("application/json");

  DPRINT("Connecting to ");
  DPRINTLN(dstHost);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < GoogleServerMaxRetry; i++) {
    int retval = client->connect(dstHost, dstPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println(F("Connection failed. Retrying..."));
  }

  if (!flag) {
    Serial.print(F("Could not connect to server: "));
    Serial.println(dstHost);
    Serial.println("Exiting...");
    delete client;
    client = nullptr;
    return;
  }

  DPRINTLN(F("GET: Fetch Google Calendar Data:"));

  // Sometimes fetching data hangs here, causing high battery drain which renders the system useless

    // Trigger HW watchdog on to long delay
  ESP.wdtDisable();
  //ESP.wdtEnable(10);
  //while (1){};

  // fetch spreadsheet data
  client->GET(dstPath, dstHost);
  String googleCalData = client->getResponseBody();

  DPRINT(F("data fetched from google: ")); DPRINTLN(googleCalData);

  // build output strings from received data

  // appointment titles
  appointment0 = getValue(googleCalData, ';', 1);
  appointment1 = getValue(googleCalData, ';', 4);
  appointment2 = getValue(googleCalData, ';', 7);
  appointment3 = getValue(googleCalData, ';', 10);

  //appointment dates
  appdate0 = getValue(googleCalData, ';', 0);
  appdate1 = getValue(googleCalData, ';', 3);
  appdate2 = getValue(googleCalData, ';', 6);
  appdate3 = getValue(googleCalData, ';', 9);

  DPRINT(F("Next appointment: ")); DPRINTLN(appointment0 + " at " + appdate0);
  DPRINT(F("Next appointment: ")); DPRINTLN(appointment1 + " at " + appdate1);
  DPRINT(F("Next appointment: ")); DPRINTLN(appointment2 + " at " + appdate2);
  DPRINT(F("Next appointment: ")); DPRINTLN(appointment3 + " at " + appdate3);

  // Appointment date strings
  //  "Thu Jul 19 2018 02:00:00 GMT+0200 (MESZ)" - substring starts at 1, not 0

  String scratchpad = appdate0.substring(0, 3);
  appDateString0 = getGermanWeekdayName(scratchpad);
  if (!appDateString0.startsWith("null")) appDateString0 += (" " + getValue(appdate0, ' ', 2) + ". " + getValue(appdate0, ' ', 1) + " um " + appdate0.substring(16, 21));
  else appDateString0 = "";

  scratchpad = appdate1.substring(0, 3);
  appDateString1 = getGermanWeekdayName(scratchpad);
  if (!appDateString1.startsWith("null")) appDateString1 += (" " + getValue(appdate1, ' ', 2) + ". " + getValue(appdate1, ' ', 1) + " um " + appdate1.substring(16, 21));
  else appDateString1 = "";

  scratchpad = appdate2.substring(0, 3);
  appDateString2 = getGermanWeekdayName(scratchpad);
  if (!appDateString2.startsWith("null")) appDateString2 += (" " + getValue(appdate2, ' ', 2) + ". " + getValue(appdate2, ' ', 1) + " um " + appdate2.substring(16, 21));
  else appDateString2 = "";

  scratchpad = appdate3.substring(0, 3);
  appDateString3 = getGermanWeekdayName(scratchpad);
  if (!appDateString3.startsWith("null")) appDateString3 += (" " + getValue(appdate3, ' ', 2) + ". " + getValue(appdate3, ' ', 1) + " um " + appdate3.substring(16, 21));
  else appDateString3 = "";

  DPRINT(F("appDateString0: ")); DPRINTLN(appDateString0);
  DPRINT(F("appDateString1: ")); DPRINTLN(appDateString1);
  DPRINT(F("appDateString2: ")); DPRINTLN(appDateString2);
  DPRINT(F("appDateString3: ")); DPRINTLN(appDateString3);

  // delete HTTPSRedirect object
  delete client;
  client = nullptr;
  DPRINT(F("Free heap at sync end .. ")); DPRINTLN(ESP.getFreeHeap());

}

//###########################################################################
// Symbols are drawn on a relative 10x15 grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale) {
  int linesize = 3;
  //Draw cloud outer
  display.fillCircle(x - scale * 3, y, scale, GxEPD_BLACK);                       // Left most circle
  display.fillCircle(x + scale * 3, y, scale, GxEPD_BLACK);                       // Right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4, GxEPD_BLACK);            // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, GxEPD_BLACK); // Right middle upper circle
  display.fillRect(x - scale * 3, y - scale, scale * 6, scale * 2 + 1, GxEPD_BLACK); // Upper and lower lines
  //Clear cloud inner
  display.fillCircle(x - scale * 3, y, scale - linesize, GxEPD_WHITE);            // Clear left most circle
  display.fillCircle(x + scale * 3, y, scale - linesize, GxEPD_WHITE);            // Clear right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4 - linesize, GxEPD_WHITE); // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, GxEPD_WHITE); // Right middle upper circle
  display.fillRect(x - scale * 3, y - scale + linesize, scale * 6, scale * 2 - linesize * 2 + 1, GxEPD_WHITE); // Upper and lower lines
}

void addrain(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 0; i < 6; i++) {
    display.drawLine(x - scale * 4 + scale * i * 1.3 + 0, y + scale * 1.9, x - scale * 3.5 + scale * i * 1.3 + 0, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.3 + 1, y + scale * 1.9, x - scale * 3.5 + scale * i * 1.3 + 1, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.3 + 2, y + scale * 1.9, x - scale * 3.5 + scale * i * 1.3 + 2, y + scale, GxEPD_BLACK);
  }
}

void addsnow(int x, int y, int scale) {
  int dxo, dyo, dxi, dyi;
  for (int flakes = 0; flakes < 5; flakes++) {
    for (int i = 0; i < 360; i = i + 45) {
      dxo = 0.5 * scale * cos((i - 90) * 3.14 / 180); dxi = dxo * 0.1;
      dyo = 0.5 * scale * sin((i - 90) * 3.14 / 180); dyi = dyo * 0.1;
      display.drawLine(dxo + x + 0 + flakes * 1.5 * scale - scale * 3, dyo + y + scale * 2, dxi + x + 0 + flakes * 1.5 * scale - scale * 3, dyi + y + scale * 2, GxEPD_BLACK);
    }
  }
}

void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 0; i < 5; i++) {
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, GxEPD_BLACK);
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, GxEPD_BLACK);
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, GxEPD_BLACK);
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, GxEPD_BLACK);
  }
}

void addsun(int x, int y, int scale) {
  int linesize = 3;
  int dxo, dyo, dxi, dyi;
  display.fillCircle(x, y, scale, GxEPD_BLACK);
  display.fillCircle(x, y, scale - linesize, GxEPD_WHITE);
  for (float i = 0; i < 360; i = i + 45) {
    dxo = 2.2 * scale * cos((i - 90) * 3.14 / 180); dxi = dxo * 0.6;
    dyo = 2.2 * scale * sin((i - 90) * 3.14 / 180); dyi = dyo * 0.6;
    if (i == 0   || i == 180) {
      display.drawLine(dxo + x - 1, dyo + y, dxi + x - 1, dyi + y, GxEPD_BLACK);
      display.drawLine(dxo + x + 0, dyo + y, dxi + x + 0, dyi + y, GxEPD_BLACK);
      display.drawLine(dxo + x + 1, dyo + y, dxi + x + 1, dyi + y, GxEPD_BLACK);
    }
    if (i == 90  || i == 270) {
      display.drawLine(dxo + x, dyo + y - 1, dxi + x, dyi + y - 1, GxEPD_BLACK);
      display.drawLine(dxo + x, dyo + y + 0, dxi + x, dyi + y + 0, GxEPD_BLACK);
      display.drawLine(dxo + x, dyo + y + 1, dxi + x, dyi + y + 1, GxEPD_BLACK);
    }
    if (i == 45  || i == 135 || i == 225 || i == 315) {
      display.drawLine(dxo + x - 1, dyo + y, dxi + x - 1, dyi + y, GxEPD_BLACK);
      display.drawLine(dxo + x + 0, dyo + y, dxi + x + 0, dyi + y, GxEPD_BLACK);
      display.drawLine(dxo + x + 1, dyo + y, dxi + x + 1, dyi + y, GxEPD_BLACK);
    }
  }
}

void addmoon(int x, int y, int scale) {
  int linesize = 3;
  int dxo, dyo, dxi, dyi;
  display.fillCircle(x, y, scale * 1.2, GxEPD_BLACK);
  display.fillCircle(x, y, scale * 1.2 - linesize, GxEPD_WHITE);

}

void addfog(int x, int y, int scale) {
  int linesize = 4;
  for (int i = 0; i < 5; i++) {
    display.fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, GxEPD_BLACK);
    display.fillRect(x - scale * 3, y + scale * 2,   scale * 6, linesize, GxEPD_BLACK);
    display.fillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, GxEPD_BLACK);
  }
}

void MostlyCloudy(int x, int y, int scale) {
  addsun(x - scale * 1.8, y - scale * 1.8, scale);
  addcloud(x, y, scale);
}

void MostlyCloudyMoon(int x, int y, int scale) {
  addsun(x - scale * 1.8, y - scale * 1.8, scale);
  addmoon(x, y, scale);
}

void MostlySunny(int x, int y, int scale) {
  addcloud(x, y, scale);
  addsun(x - scale * 1.8, y - scale * 1.8, scale);
}

void MostlyMoon(int x, int y, int scale) {
  addcloud(x, y, scale);
  addmoon(x - scale * 1.8, y - scale * 1.8, scale);
}

void Rain(int x, int y, int scale) {
  addcloud(x, y, scale);
  addrain(x, y, scale);
}

void Cloudy(int x, int y, int scale) {
  addcloud(x, y, scale);
}

void Sunny(int x, int y, int scale) {
  scale = scale * 1.5;
  addsun(x, y, scale);
}

void Moon(int x, int y, int scale) {
  scale = scale * 1.5;
  addmoon(x, y, scale);
}

void ExpectRain(int x, int y, int scale) {
  addsun(x - scale * 1.8, y - scale * 1.8, scale);
  addcloud(x, y, scale);
  addrain(x, y, scale);
}

void Tstorms(int x, int y, int scale) {
  addcloud(x, y, scale);
  addtstorm(x, y, scale);
}

void Snow(int x, int y, int scale) {
  addcloud(x, y, scale);
  addsnow(x, y, scale);
}

void Fog(int x, int y, int scale) {
  addcloud(x, y, scale);
  addfog(x, y, scale);
}

void Nodata(int x, int y, int scale) {
  if (scale == 10) display.setTextSize(3); else display.setTextSize(1);
  display.setCursor(x, y);
  display.println("?");
  display.setTextSize(1);
}

/* Code from stackoverflow
  https://stackoverflow.com/a/14824108
*/

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String getGermanWeekdayName(String &weekd) {

  String weekdg;

  if (weekd == "Mon") weekdg = weekDay0;
  else if (weekd == "Tue") weekdg = weekDay1;
  else if (weekd == "Wed") weekdg = weekDay2;
  else if (weekd == "Thu") weekdg = weekDay3;
  else if (weekd == "Fri") weekdg = weekDay4;
  else if (weekd == "Sat") weekdg = weekDay5;
  else if (weekd == "Sun") weekdg = weekDay6;
  else weekdg = "null";

  return weekdg;
}

// get UTC time referenced to 1970 by NTP
time_t getNTP_UTCTime1970()
{
  bNTPStarted = false; // invalidate; time-lib functions crash, if not initalized poperly
  unsigned long t = getNTPTimestamp();
  if (t == 0) return (0);

  // scale to 1970
  // may look like back & forth with ntp code; wrote it to make needed conversions more clear
  return (t + 946684800UL);
}


int getADCRAW() {
  // time to read battery voltage at 5V pin
  unsigned int batVoltageRAW = analogRead(A0); // read voltage at A0
  return batVoltageRAW;
}

float getAnalogVoltage(int raw) {

  // voltage divider is 470k/100k, so resistance ratio is 4.7
  // 1V = 1024

  unsigned int upperRes = 470; // kOhm
  unsigned int lowerRes = 100; // kOhm


  float batVoltage = (((float)upperRes + lowerRes) / 100) * ((float)raw / 1024);

  Serial.print(F("Battery Voltage: ")); Serial.print(raw); Serial.print(F("digit or ")); Serial.print(batVoltage); Serial.println(F(" volts."));

  return batVoltage;

}

//###########################################################################



