/**

 * @file ESP32-Clock-Display.cpp
 * @brief ESP32 Clock mit NTP Sync einmal täglich, Modem-Sleep und OLED-Anzeige
 *
 * - NTP-Sync beim Start + jeden Tag um 05:00 Uhr
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
 * - secrets.h for WiFi credentials
 *
 * 
 * Author: Wolfgang-Berlin
 * License: MIT License
 * Date: 24-08-2025
 

                 |    |
                 |    |
  -        -      ----       -          -
  -        -                 -          -
  -        -                 - + 3.3 V  -
  -    GND -                 -          -
  -        -  D1 Mini ESP32  -          -
  -        -                 -          -
  -    SDA -                 -          -
  -    SCK -                 -          -
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

const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R2);

// Berlin/Europa mit DST
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"

static int lastDisplayedMinute = -1; 
static int lastSyncDay = -1;

// --- WiFi sauber trennen ---
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
  strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);
  oled.clearBuffer();
  oled.setContrast(32);
  oled.setFont(u8g2_font_logisoso42_tr);
  oled.drawStr(1, 52, timeStr);
  oled.sendBuffer();
  Serial.printf("Anzeige: %s\n", timeStr);
}

// --- NTP Synchronisation ---
bool syncTime() {
  Serial.println("NTP-Sync starten…");
  showStatus("WLAN an…");

  WiFi.disconnect(true, true);
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

  configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov", "time.google.com");
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

  Serial.print("Neue Uhrzeit: ");
  Serial.println(asctime(&ti));
  showStatus("Zeit OK");
  delay(1000);

  disconnectWiFi();
  lastSyncDay = ti.tm_yday;

  return true;
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  oled.begin();
  oled.setPowerSave(0);

  // erster NTP-Sync beim Start
  syncTime();

  // Energiesparmodus
  setCpuFrequencyMhz(20);
  WiFi.setSleep(true);
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

// --- Loop ---
void loop() {
  time_t now = time(nullptr);
  struct tm nowLocal;
  localtime_r(&now, &nowLocal);

  // Einmal täglich um 05:00 → Zeit synchronisieren
  if (nowLocal.tm_hour == 5 && lastSyncDay != nowLocal.tm_yday) {
    if (syncTime()) {
      Serial.println("Täglicher NTP-Sync erledigt");
    }
  }

  // Anzeige nur zwischen 06:00 und 21:59
  if (nowLocal.tm_hour >= 6 && nowLocal.tm_hour < 22) {
    oled.setPowerSave(0);
    if (nowLocal.tm_min != lastDisplayedMinute) {
      drawTime(&nowLocal);
      lastDisplayedMinute = nowLocal.tm_min;
    }
  } else {
    // Display aus in der Nacht
    oled.setPowerSave(1);
    lastDisplayedMinute = -1;
  }

  delay(1000); // 1 s Pause
}
