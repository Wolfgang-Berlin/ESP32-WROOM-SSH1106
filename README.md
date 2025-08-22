


This code implements a low-power clock using an ESP32 microcontroller, an OLED display (SH1106), and NTP (Network Time Protocol) for accurate time synchronization. The main goal is to display the current time during the day and enter deep sleep mode at night to conserve energy. The device connects to WiFi only briefly to synchronize its clock, then disconnects to save power.

At startup, the ESP32 initializes the serial interface and OLED display. It then attempts to synchronize the time using NTP servers, connecting to WiFi with credentials stored in a separate secrets.h file. If synchronization is successful, the current time is displayed on the OLED. The code uses the Berlin/Europe timezone, including daylight saving adjustments.

The main loop checks the current local time. If the time is between 22:00 (10 PM) and 06:00 (6 AM), the device enters deep sleep until 6 AM, turning off the display and minimizing power consumption. During the day, the clock updates the display only when the minute changes, reducing unnecessary redraws and further saving energy.

Additional power-saving features include lowering the CPU frequency and enabling modem sleep when WiFi is off. The code is structured for clarity, with separate functions for WiFi management, status display, time drawing, NTP synchronization, and sleep scheduling. This makes it easy to maintain and extend for similar low-power IoT applications.




Dieser Code implementiert eine stromsparende Uhr mit einem ESP32-Mikrocontroller, einem OLED-Display (SH1106) und NTP (Network Time Protocol) für eine präzise Zeitsynchronisierung. Das Hauptziel ist die Anzeige der aktuellen Uhrzeit tagsüber und der Wechsel in den Tiefschlafmodus nachts, um Energie zu sparen. Das Gerät verbindet sich nur kurz mit dem WLAN, um seine Uhr zu synchronisieren, und trennt die Verbindung anschließend, um Strom zu sparen.

Beim Start initialisiert der ESP32 die serielle Schnittstelle und das OLED-Display. Anschließend versucht er, die Uhrzeit über NTP-Server zu synchronisieren. Dabei verbindet er sich mit den in einer separaten secrets.h-Datei gespeicherten Anmeldedaten mit dem WLAN. Bei erfolgreicher Synchronisierung wird die aktuelle Uhrzeit auf dem OLED-Display angezeigt. Der Code verwendet die Zeitzone Berlin/Europa, einschließlich Sommerzeitanpassungen.

Die Hauptschleife prüft die aktuelle Ortszeit. Liegt die Uhrzeit zwischen 22:00 Uhr und 6:00 Uhr, wechselt das Gerät bis 6:00 Uhr in den Tiefschlaf, schaltet das Display aus und minimiert so den Stromverbrauch. Tagsüber aktualisiert die Uhr die Anzeige nur zum Minutenwechsel. Dadurch werden unnötige Neuzeichnungen vermieden und Energie gespart.

Zu den weiteren Energiesparfunktionen gehören die Reduzierung der CPU-Frequenz und die Aktivierung des Modem-Ruhezustands bei ausgeschaltetem WLAN. Der Code ist übersichtlich strukturiert und bietet separate Funktionen für WLAN-Verwaltung, Statusanzeige, Zeiterfassung, NTP-Synchronisierung und Ruhezustandsplanung. Dies erleichtert die Wartung und Erweiterung für ähnliche stromsparende IoT-Anwendungen.


# ESP32-WROOM-SSH1106
