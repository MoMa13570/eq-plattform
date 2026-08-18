# EQ Plattform

Eine DIY-Nachführplattform für Dobson-Teleskope mit Arduino Uno, Nano oder
ESP32, NEMA17-Schrittmotor und TMC2209-Treiber. Die ESP32-Firmware bringt
zusätzlich einen eigenen WLAN-Hotspot und eine responsive Websteuerung mit.

## Direkt starten

- [Projekt-Website und Bauübersicht](https://moma13570.github.io/eq-plattform/)
- [Firmware im Browser installieren](https://moma13570.github.io/eq-plattform/flasher/)
- [Aktuelle Firmware herunterladen](https://github.com/MoMa13570/eq-plattform/releases/latest)
- [Bauanleitung als PDF](build_instructions/buildinstruction_EQ_Plattform.pdf)
- [Stückliste als CSV](part%20list/Teileliste_DM%20EQ%20Plattform.csv)

Der Browser-Flasher benötigt Chrome oder Edge auf einem Desktop-Rechner. Die
HEX-Datei wird lokal verarbeitet und nicht an einen Server übertragen.

## Inhalt

- `hardware/mechanical/` – STEP-Dateien für Standard- und Simple-Mechanik
- `hardware/circuitboard/` – Schaltplan, PCB-Daten und Elektronikgehäuse
- `software/EQ-Plattform_TMC2209/` – PlatformIO-Projekt und Firmware-Quellcode
- `part list/` – Stückliste
- `build_instructions/` – ausführliche Bauanleitung
- `flasher/` – statischer Web-Serial-Flasher

## Firmware selbst kompilieren

Das PlatformIO-Projekt unterstützt Arduino Uno, Nano und ESP32:

```text
software/EQ-Plattform_TMC2209/
```

Die wichtigsten Umgebungen in `platformio.ini` sind `uno`, `nano` und `esp32`.
Details zur ESP32-Pinbelegung und Web-App stehen in der
[Software-Dokumentation](software/README.md). Für Uno/Nano ist der fertige
HEX-Download im aktuellen GitHub Release einfacher.

## Lizenz

Siehe [LICENSE.rtf](LICENSE.rtf).
