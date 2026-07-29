# Firmware

## Einfach installieren

1. Die aktuelle HEX-Datei im
   [GitHub Release](https://github.com/MoMa13570/eq-plattform/releases/latest)
   herunterladen.
2. Den
   [EQ Firmware Flasher](https://moma13570.github.io/eq-plattform/flasher/)
   in Chrome oder Edge öffnen.
3. Board und HEX-Datei auswählen, Arduino verbinden und installieren.

Die veröffentlichte Firmware ist für ATmega328P-Boards ausgelegt. Dasselbe
Firmware-Abbild funktioniert auf Arduino Uno und Nano; im Flasher wird das
Board ausgewählt, damit das passende Bootloader-Protokoll verwendet wird.

## Quellcode

Das PlatformIO-Projekt befindet sich in
[`EQ-Plattform_TMC2209/`](EQ-Plattform_TMC2209/). Die Umgebungen `uno` und
`nano` bauen die aktuelle Firmware mit Endstop- und Homing-Unterstützung.
