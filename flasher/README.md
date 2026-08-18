# EQ Firmware Flasher

Statische Chrome-Web-App zum Übertragen einer Intel-HEX-Firmware auf:

- Arduino Uno
- Arduino Nano
- Arduino Nano (alter Bootloader)
- ESP32 DevKit V1

Die App verwendet die Web Serial API direkt im Browser. Es gibt kein Backend,
kein Benutzerkonto und keinen Upload der Firmware an einen Server.

Für Uno/Nano verarbeitet der Flasher Intel-HEX und spricht das STK500v1-
Bootloaderprotokoll. Für den ESP32 wird ein zusammengeführtes Factory-BIN ab
Adresse `0x0` mit Espressifs `esptool-js` übertragen. Die passenden fertigen
Dateien liegen im Verzeichnis [`release/`](../release/).

`esptool-js` 0.5.7 ist lokal unter `vendor/` eingebunden, sodass der Flasher
zur Laufzeit keine Bibliothek von einem externen CDN nachladen muss. Die
Apache-2.0-Lizenz liegt daneben.

Nach der Veröffentlichung über GitHub Pages:

<https://moma13570.github.io/eq-plattform/flasher/>

Voraussetzungen: Chrome oder Edge auf einem Desktop-Rechner sowie eine
HTTPS-Verbindung (bei GitHub Pages automatisch gegeben).
