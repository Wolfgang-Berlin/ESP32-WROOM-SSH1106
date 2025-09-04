/**

 * @file ESP32-Clock-Display.cpp
 * @brief ESP32 Clock mit NTP Sync einmal täglich, Modem-Sleep und OLED-Anzeige
 *
 * - NTP-Sync beim Start + jeden Tag um 04:30 Uhr
 * - WLAN danach ausschalten, Energiesparen mit Modem-Sleep
 * - Anzeige jede Minute von 06:00–22:00 Uhr
 * - Anzeige aus zwischen 22:00–06:00 Uhr
 *
 * Hardware: ESP32 + SH1106 OLED (U8g2)
 * 
 * Dependencies:
 * - Arduino core for ESP32
 * - U8g2 library for OLED display
 *           https://github.com/olikraus/u8g2/wiki/fntlistallplain#u8g2-font-list
 * - WiFi library for network connectivity
 * - create file "secrets.h" for WiFi credentials
 * 
 *          #define SECRET_SSID "..."		  // replace MySSID with your WiFi network name
            #define SECRET_PASS "XXXXXX"	// replace MyPassword with your WiFi password

 * 
 * Author: Wolfgang-Berlin
 * License: MIT License
 * Date: 29-08-2025
 

                 |    |
                 |    |
  -        -      ----       -          -
  -        -                 -          -
  -        -                 - + 3.3 V  -
  -    GND -                 -          -
  -        -  D1 Mini ESP32  -          -
  -        -                 -          -
  -    SDA -  GPIO 21        -          -
  -    SCK -  GPIO 22        -          -
  -        -                 -          -
  -        -                 -          -


*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <secrets.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <time.h>

# define oled_CLK 22
# define oled_SDA 21

const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R2, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ oled_CLK, /* data=*/ oled_SDA); // SH1106 128x64 via I2C

// Berlin/Europa mit DST
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/3"

static int lastDisplayedMinute = -1; 
static int lastSyncDay = -1;

// --- WiFi trennen ---
void disconnectWiFi() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WLAN aus");
}

// --- OLED Statusmeldung ---
void showStatus(const char* msg) {
  oled.setPowerSave(0);
  oled.clearBuffer();
  oled.setContrast(64);
  oled.setFont(u8g2_font_courR08_tr);
  oled.drawStr(0, 60, msg);
  oled.sendBuffer();
}

// --- Zeit anzeigen ---
void drawTime(const struct tm* timeinfo) {
  char timeStr[6];
  oled.setPowerSave(0);
  oled.clearBuffer();
  strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);
  oled.setContrast(30);
  oled.setFont(u8g2_font_logisoso42_tr);
  oled.drawStr(1, 52, timeStr);
  oled.sendBuffer();
}

// --- NTP Synchronisation ---
bool syncTime() {

  Serial.println("NTP-Sync starten…");
  showStatus("WLAN an…");
  esp_wifi_set_ps(WIFI_PS_NONE); // aufwachen - WLAN volle Leistung (kein Sleep)
  setCpuFrequencyMhz(160); // CPU auf 160 MHz
  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WLAN Timeout");
    showStatus("WLAN Timeout");
    disconnectWiFi();
    return false;
  }

  Serial.println("WLAN verbunden");
  showStatus("NTP Sync…");

  configTzTime(TIMEZONE, "de.pool.ntp.org");
  tzset();

  time_t now = 0;
  struct tm ti = {0};
  bool ok = false;
  for (int i = 0; i < 60; ++i) {
    time(&now);
    localtime_r(&now, &ti);
    if (ti.tm_year >= (2024 - 1900)) { ok = true; break; }
    delay(500);
  }

  if (!ok) {
    Serial.println("NTP fehlgeschlagen");
    showStatus("NTP fehlgeschlagen");
    disconnectWiFi();
    return false;
  }

  showStatus("Zeit OK");
  delay(1000);

  disconnectWiFi();
  setCpuFrequencyMhz(40); // wieder auf 40 MHz runter, spart Strom
  WiFi.setSleep(true);
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM); // Modem-Sleep

  return true;
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  oled.begin();
  oled.setPowerSave(0); // Display an
  oled.setContrast(64);
  oled.clearBuffer();
  if (WiFi.status() == WL_CONNECTED) {
    showStatus("NTP-Sync…");
  }
  // erster NTP-Sync beim Start
  syncTime();
  delay(500);
}

// --- Loop ---

static bool syncDoneThisMinute = false;
#define Sync_Stunde 4         // rechtzeitig vor 6 Uhr synchronisieren: Zeitumstellung muss so nicht beachtet werden
#define Sync_Min    30
const int sleepTime_Start = 22;  // 22:00 Uhr
const int sleepTime_End  =  6;   // 06:00 Uhr

void loop() {
  time_t now = time(nullptr);
  struct tm nowLocal;
  localtime_r(&now, &nowLocal);

   if (nowLocal.tm_hour >= Sync_Stunde && nowLocal.tm_min == Sync_Min && !syncDoneThisMinute ) { //
    
      if (syncTime()) {
        Serial.println("Täglicher NTP-Sync erfolgreich");
      } else {
            Serial.println("Täglicher NTP-Sync fehlgeschlagen, neuer Versuch in 5 Min");
        }
      syncDoneThisMinute = true;
  }
  if (nowLocal.tm_min != Sync_Min) {
    syncDoneThisMinute = false;
  }

  if (nowLocal.tm_hour >= sleepTime_Start || nowLocal.tm_hour < sleepTime_End) {
    // Zwischen 22:00 und 06:00 Uhr
      if (nowLocal.tm_min != lastDisplayedMinute) { //
          oled.setPowerSave(1); 
          oled.clearBuffer();
          oled.sendBuffer();
          lastDisplayedMinute = nowLocal.tm_min;
      }

    } else {
    // Tageszeit 06:00–22:00 Uhr
      oled.setPowerSave(0);
      if (nowLocal.tm_min != lastDisplayedMinute) { 
          drawTime(&nowLocal);
          lastDisplayedMinute = nowLocal.tm_min;
      }  
  }
  delay(1000); // 1 s Pause
}
