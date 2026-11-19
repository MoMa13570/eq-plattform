# Firmware

## Einfach installieren

1. Die aktuelle HEX-Datei im
   [GitHub Release](https://github.com/MoMa13570/eq-plattform/releases/latest)
   herunterladen.
2. Den
   [EQ Firmware Flasher](https://moma13570.github.io/eq-plattform/flasher/)
   in Chrome oder Edge öffnen.
3. Board und HEX-Datei auswählen, Arduino verbinden und installieren.

Für den ESP32 im Flasher `ESP32 DevKit` wählen und die mitgelieferte
`EQ-Plattform-ESP32.factory.bin` verwenden. Der Flasher kann sie direkt laden;
ein vorheriger Download ist nicht nötig.

Die veröffentlichte Firmware ist für ATmega328P-Boards ausgelegt. Dasselbe
Firmware-Abbild funktioniert auf Arduino Uno und Nano; im Flasher wird das
Board ausgewählt, damit das passende Bootloader-Protokoll verwendet wird.

## ESP32 mit Web-App

Zusätzlich gibt es die PlatformIO-Umgebung `esp32` für ein ESP32 DevKit V1.
Sie enthält dieselbe Steuerung einschließlich OLED, Endschaltern, Tracking,
Mondrate und Homing. Der ESP32 öffnet außerdem den Hotspot `EQ-Plattform`
(Passwort `eqplattform`). Nach dem Verbinden ist die Steuerung unter
`http://192.168.4.1` erreichbar. Auf vielen Smartphones öffnet sich die
Web-App durch die integrierte Captive-Portal-Erkennung automatisch.

Die Web-App zeigt den Live-Status und bietet:

- Tracking starten/stoppen, Richtung Nord/Süd und Mondrate
- Homing und Sofort-Stopp
- konfigurierbaren Poti-Bereich oder Betrieb ohne Poti
- getrennte TMC2209-Ströme für Tracking, schnelles und langsames Homing
- einstellbare Homing-Geschwindigkeiten

Alle Einstellungen werden im Flash des ESP32 gespeichert.

### Heim-WLAN einrichten

1. Mit dem Hotspot `EQ-Plattform` verbinden und `http://192.168.4.1` im
   Browser öffnen.
2. Unter **Heim-WLAN** auf **Nach WLAN suchen** klicken, das
   2,4-GHz-Netz auswählen und bei Bedarf das WLAN-Passwort eingeben.
3. **Verbinden** wählen. Die Web-App zeigt die Heimnetz-IP an. Sie steht
   auch in der Zeile unter der Hotspot-IP auf dem OLED.

Nach erfolgreicher Verbindung speichert der ESP32 das WLAN-Profil und
verbindet sich beim nächsten Start automatisch. Der Hotspot bleibt auch ohne
Heimnetz verfügbar. Ein fehlgeschlagener Verbindungsversuch mit neuen
Zugangsdaten lässt das bisher gespeicherte Profil bestehen.

### Firmware per WLAN aktualisieren (OTA)

1. Eine Firmware mit OTA-Unterstützung einmal per USB installieren.
2. Mit dem Hotspot `EQ-Plattform` verbinden und `http://192.168.4.1` im
   Browser öffnen.
3. Nachführung oder Homing stoppen und unter **Firmware aktualisieren** die
   Datei `EQ-Plattform-ESP32.ota.bin` aus dem GitHub Release auswählen.
   Bei einem lokalen Build mit `pio run -e esp32` liegt die passende Datei
   unter `.pio/build/esp32/firmware.bin`.
4. **Update installieren** wählen und den ESP32 eingeschaltet lassen. Nach
   erfolgreichem Update startet er neu. Danach bei Bedarf erneut mit dem
   Hotspot verbinden.

Die `.factory.bin` ist für den USB-Flasher vorgesehen. Beim OTA-Update
bleiben die gespeicherten Einstellungen erhalten. Geräte im ESP32-Hotspot
können die Update-Funktion verwenden.

### Pinbelegung ESP32 DevKit V1

| Funktion | GPIO |
| --- | ---: |
| STEP / DIR / EN | 25 / 26 / 27 |
| Poti | 34 |
| Richtung Nord / Süd | 32 / 33 |
| Home-Taster | 13 |
| Endschalter Home / Ende | 18 / 19 |
| TMC2209 UART RX / TX | 16 / 17 |
| OLED SDA / SCL | 21 / 22 |

Taster, Richtungsschalter und Endschalter schalten gegen GND. Der ESP32 ist
nicht 5-V-tolerant; alle Signale müssen mit 3,3 V kompatibel sein. Die Masse
von ESP32, TMC2209 und Motorversorgung muss verbunden werden.

Beim üblichen BTT TMC2209 V1.3 werden GPIO 16 (RX) direkt und GPIO 17 (TX)
über einen 1-kΩ-Widerstand gemeinsam an `PDN_UART` angeschlossen. Die
eingestellten RMS-Ströme sind Sicherheitsgrenzen der Software, ersetzen aber
nicht die Prüfung, ob Motor, Treiber und Kühlung dafür ausgelegt sind.

Kompilieren und hochladen:

```bash
cd software/EQ-Plattform_TMC2209
pio run -e esp32
pio run -e esp32 -t upload
```

Fertige Images für den Web-Flasher liegen unter [`release/`](../release/).
Für den ESP32 muss dort die Datei mit der Endung `.factory.bin` verwendet
werden; die normale PlatformIO-Datei `firmware.bin` enthält nur die Anwendung.

## Quellcode

Das PlatformIO-Projekt befindet sich in
[`EQ-Plattform_TMC2209/`](EQ-Plattform_TMC2209/). Die Umgebungen `uno`, `nano`
und `esp32` bauen die jeweilige Firmware mit Endstop- und Homing-Unterstützung.
