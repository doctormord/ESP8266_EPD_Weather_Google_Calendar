/*-----------------------------------------------------------
NTP & time routines for ESP8266 
    for ESP8266 adapted Arduino IDE

by Stefan Thesen 05/2015 - free for anyone

code for ntp adopted from Michael Margolis
code for time conversion based on http://stackoverflow.com/
-----------------------------------------------------------*/

// note: all timing relates to 01.01.2000 otherwise function name indicates differently

#ifndef _DEF_TIME_NTP_ST_
#define _DEF_TIME_NTP_ST_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

typedef struct
{
    unsigned char second; // 0-59
    unsigned char minute; // 0-59
    unsigned char hour;   // 0-23
    unsigned char day;    // 1-31
    unsigned char month;  // 1-12
    unsigned char year;   // 0-99 (representing 2000-2099)
}
date_time_t;

unsigned long getNTPTimestamp();
unsigned long sendNTPpacket(IPAddress& address);
unsigned int date_time_to_epoch(date_time_t* date_time);
void epoch_to_date_time(date_time_t* date_time,unsigned int epoch);
String epoch_to_string(unsigned int epoch);

#endif
