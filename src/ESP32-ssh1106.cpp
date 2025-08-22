/**
 * @file ESP32-ssh1106.cpp
 * @brief ESP32 Deep-Sleep Clock with NTP Synchronization and OLED Display
 *
 * This program implements a low-power clock using an ESP32 microcontroller, an SH1106 OLED display, and NTP for time synchronization.
 * 
 * Features:
 * - Connects to WiFi for NTP time sync, then disconnects to save power.
 * - Displays current time on OLED using U8g2 library.
 * - Enters deep sleep mode during night hours (22:00–06:00) and wakes up at 06:00.
 * - Uses Berlin/Europe timezone with DST support.
 * - OLED display is turned off during sleep and wakes up to show time.
 * - WiFi is only enabled for synchronization, then disabled for power saving.
 * - Modem sleep is enabled for further power reduction.
 *
 * Dependencies:
 * - Arduino core for ESP32
 * - U8g2 library for OLED display
 *           https://github.com/olikraus/u8g2/wiki/fntlistallplain#u8g2-font-list
 * - WiFi library for network connectivity
 * - secrets.h for WiFi credentials
 *
 * 
 * Author: Your Name
 * License: MIT License
 * Date: 21-08-2025
 

                |    |
                |    |
          -                 -
          -                 -
          -                 -   + 3.3 V
    GND   -                 -   
          -  D1 Mini ESP32  -
          -                 -
    SDA   -                 -
    SCK   -                 -
          -                 -
          -                 -


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

// --- WLAN trennen ---
void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WLAN getrennt");
}

// --- Statusanzeige ---
void showStatus(const char* msg) {
  oled.setPowerSave(0);
  oled.begin();
  oled.clearBuffer();
  oled.setContrast(64);   // Helligkeit
  oled.setFont(u8g2_font_courR08_tr);
  oled.drawStr(0, 60, msg);
  oled.sendBuffer();
}

// --- Uhrzeit anzeigen ---
void drawTime(const struct tm* timeinfo) {
  char timeStr[6];
  strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);
  oled.clearBuffer();
  oled.setContrast(32);
  oled.setFont(u8g2_font_logisoso42_tr);
  oled.drawStr(1, 52, timeStr);
  oled.sendBuffer();
  Serial.print("Anzeige: ");
  Serial.println(timeStr);
}

// --- Zeit mit NTP synchronisieren ---
bool syncTime() {
  Serial.println("NTP-Sync starten…");
  showStatus("WLAN einschalten…");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WLAN Timeout");
    showStatus("WLAN Timeout");
    disconnectWiFi();
    return false;
  }

  Serial.println("WLAN verbunden");
  showStatus("WLAN verbunden");

  Serial.println("Starte NTP-Sync…");
  showStatus("NTP-Sync…");
  configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov", "time.google.com");
  tzset();

  time_t now = 0;
  struct tm ti = {0};
  bool ok = false;
  for (int i = 0; i < 30; ++i) {
    time(&now);
    localtime_r(&now, &ti);
    if (ti.tm_year >= (2024 - 1900)) { ok = true; break; }
    delay(500);
  }

  if (!ok) {
    Serial.println("NTP-Sync fehlgeschlagen");
    showStatus("NTP fehlgeschlagen");
    disconnectWiFi();
    return false;
  }

  Serial.println("NTP-Sync erfolgreich");
  showStatus("Zeit OK, WLAN aus");
  disconnectWiFi();

  lastDisplayedMinute = -1;
  drawTime(&ti);

  return true;
}

// --- Deep-Sleep bis 06:00 ---
void sleepUntilSix(const struct tm* nowLocal) {
  oled.setPowerSave(1); // Display aus
  Serial.println("Nachtruhe: Deep-Sleep bis 06:00…");


  struct tm wake = *nowLocal;
  if (wake.tm_hour >= 22) {
    wake.tm_mday += 1;
  }
  wake.tm_hour = 6;
  wake.tm_min  = 0;
  wake.tm_sec  = 0;

  time_t nowEpoch  = mktime((struct tm*)nowLocal);
  time_t wakeEpoch = mktime(&wake);
  uint64_t sleepSeconds = (uint64_t)difftime(wakeEpoch, nowEpoch);
  if (sleepSeconds < 1) sleepSeconds = 1;

  esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
  esp_deep_sleep_start();
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  oled.begin();

  // NTP-Sync beim Start (z. B. morgens nach Deep-Sleep)
  syncTime();
  setCpuFrequencyMhz(20);
  // Modem-Sleep aktivieren (WLAN ist ja aus, CPU läuft energiesparend)
  WiFi.setSleep(true);
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}


// --- Loop ---
void loop() {
  time_t now = time(nullptr);
  struct tm nowLocal;
  localtime_r(&now, &nowLocal);

  // Nachtruhe 22–06 → Deep-Sleep
  if (nowLocal.tm_hour >= 22 || nowLocal.tm_hour < 6) {
    sleepUntilSix(&nowLocal);
    return;
  }

  // Minutenanzeige
  if (nowLocal.tm_min != lastDisplayedMinute) { 
    drawTime(&nowLocal);
    lastDisplayedMinute = nowLocal.tm_min;
  }

  // Modem-Sleep-Phase → nur kleine Pause
  delay(1000);
}
