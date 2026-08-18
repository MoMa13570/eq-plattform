# Fertige Firmware

- `EQ-Plattform-UNO-NANO.hex` – Arduino Uno oder Nano mit ATmega328P
- `EQ-Plattform-ESP32.factory.bin` – vollständiges ESP32 DevKit V1 Image

Beide Dateien können direkt im [Web-Flasher](../flasher/index.html) verwendet
werden. Das ESP32-Factory-Image enthält Bootloader, Partitionstabelle und
Anwendung und wird ab Flash-Adresse `0x0` installiert.

Bei einem Git-Tag wie `v1.0.0` baut der Workflow `firmware-release.yml` beide
Dateien erneut und hängt sie zusammen mit `SHA256SUMS.txt` an den GitHub
Release. Er kann außerdem manuell als Workflow gestartet werden.

## SHA-256

```text
a018c9d37cf94f3ed3204fbee39d7b598d2756b0838089d841da10bda20a1d82  EQ-Plattform-UNO-NANO.hex
f4a5176cb3ebf21efc48b64490342c9812eb65955f2bdc9ecd68fb28551eea28  EQ-Plattform-ESP32.factory.bin
```
