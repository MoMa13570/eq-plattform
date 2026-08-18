const ESPTOOL_JS_URL = "./vendor/esptool-js.bundle.js";

function uint8ToBinaryString(bytes) {
  const chunks = [];
  const chunkSize = 0x8000;
  for (let offset = 0; offset < bytes.length; offset += chunkSize) {
    chunks.push(
      String.fromCharCode(...bytes.subarray(offset, offset + chunkSize)),
    );
  }
  return chunks.join("");
}

export function parseEsp32FactoryBin(buffer) {
  const bytes = new Uint8Array(buffer);
  if (bytes.length < 0x1100) {
    throw new Error("Das ESP32-Factory-Image ist unvollständig.");
  }
  if (bytes.length > 4 * 1024 * 1024) {
    throw new Error("Das ESP32-Image ist größer als der 4-MB-Flash.");
  }
  // A merged ESP32 image starts at address 0 and contains the bootloader at 0x1000.
  if (bytes[0x1000] !== 0xe9) {
    throw new Error(
      "Keine ESP32-Factory-BIN erkannt. Bitte das Image mit der Endung .factory.bin verwenden.",
    );
  }
  return {
    kind: "esp32",
    bytes,
    usedBytes: bytes.length,
    highestAddress: bytes.length,
  };
}

export async function flashEsp32Firmware({
  port,
  firmware,
  onLog,
  onProgress,
}) {
  onLog("Lade Espressif-Flashtreiber …");
  const { ESPLoader, Transport } = await import(ESPTOOL_JS_URL);
  const transport = new Transport(port, true);
  const terminal = {
    clean() {},
    write(data) {
      const message = String(data).trim();
      if (message) onLog(message);
    },
    writeLine(data) {
      const message = String(data).trim();
      if (message && !message.startsWith("Writing at ")) onLog(message);
    },
  };
  const loader = new ESPLoader({
    transport,
    baudrate: 460800,
    terminal,
    debugLogging: false,
  });

  try {
    onLog("Verbinde mit dem ESP32-Bootloader …");
    const chip = await loader.main();
    if (!String(chip).toUpperCase().includes("ESP32")) {
      throw new Error(`Nicht unterstützter Espressif-Chip: ${chip}.`);
    }
    onLog(`Chip erkannt: ${chip}.`);
    await loader.writeFlash({
      // esptool-js 0.5.x expects a binary string, not a Uint8Array.
      fileArray: [{ data: uint8ToBinaryString(firmware.bytes), address: 0x0 }],
      flashMode: "dio",
      flashFreq: "40m",
      flashSize: "4MB",
      eraseAll: false,
      compress: true,
      reportProgress(_fileIndex, written, total) {
        onProgress(Math.min(100, Math.round((written / total) * 100)));
      },
    });
    onProgress(100);
    onLog("Factory-Image geschrieben und geprüft.");
    await loader.after("hard_reset");
  } finally {
    await transport.disconnect().catch(() => {});
  }
}
