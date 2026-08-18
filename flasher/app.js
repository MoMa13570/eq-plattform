import {
  BOARD_PROFILES,
  describePort,
  flashFirmware,
  parseIntelHex,
  requestArduinoPort,
} from "./firmware.js";
import { flashEsp32Firmware, parseEsp32FactoryBin } from "./esp32.js";

const BOARDS = [
  ...BOARD_PROFILES.map((board) => ({
    ...board,
    family: "avr",
    extension: ".hex",
    releaseFile: "EQ-Plattform-UNO-NANO.hex",
  })),
  {
    id: "esp32",
    name: "ESP32 DevKit V1",
    detail: "WLAN + Web-App · Factory BIN",
    family: "esp32",
    extension: ".bin",
    maxFirmwareBytes: 4 * 1024 * 1024,
    releaseFile: "EQ-Plattform-ESP32.factory.bin",
  },
];

const elements = {
  browserDot: document.querySelector("#browser-dot"),
  browserStatus: document.querySelector("#browser-status"),
  unsupportedBanner: document.querySelector("#unsupported-banner"),
  boardGrid: document.querySelector("#board-grid"),
  fileInput: document.querySelector("#firmware-input"),
  fileName: document.querySelector("#file-name"),
  fileDetail: document.querySelector("#file-detail"),
  fileIcon: document.querySelector("#file-icon"),
  dropzone: document.querySelector("#dropzone"),
  bundledFirmware: document.querySelector("#bundled-firmware"),
  releaseDownload: document.querySelector("#release-download"),
  connectButton: document.querySelector("#connect-button"),
  connectState: document.querySelector("#connect-state"),
  flashButton: document.querySelector("#flash-button"),
  statusSymbol: document.querySelector("#status-symbol"),
  statusTitle: document.querySelector("#status-title"),
  statusDetail: document.querySelector("#status-detail"),
  progressTrack: document.querySelector("#progress-track"),
  progressBar: document.querySelector("#progress-bar"),
  progressText: document.querySelector("#progress-text"),
  log: document.querySelector("#log"),
};

const state = {
  board: BOARDS[0],
  firmware: null,
  port: null,
  working: false,
  phase: "idle",
};

const webSerialSupported = "serial" in navigator;
elements.browserStatus.textContent = webSerialSupported
  ? "Web Serial verfügbar"
  : "Browser nicht unterstützt";
elements.browserDot.classList.toggle("unsupported", !webSerialSupported);
elements.unsupportedBanner.hidden = webSerialSupported;
elements.connectButton.disabled = !webSerialSupported;

function now() {
  return new Intl.DateTimeFormat("de-DE", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  }).format(new Date());
}

function log(message) {
  const line = document.createElement("p");
  const time = document.createElement("time");
  time.textContent = now();
  line.append(time, document.createTextNode(message));
  elements.log.append(line);
  elements.log.scrollTop = elements.log.scrollHeight;
}

function setStatus(phase) {
  state.phase = phase;
  const statuses = {
    idle: ["·", "Warte auf Auswahl", "Drei kurze Schritte"],
    ready: ["→", "Bereit zum Installieren", "Alle Angaben vollständig"],
    working: ["↧", "Firmware wird installiert", "Board nicht trennen"],
    success: ["✓", "Installation abgeschlossen", "Board ist einsatzbereit"],
    error: ["!", "Installation fehlgeschlagen", "Details im Protokoll"],
  };
  const [symbol, title, detail] = statuses[phase];
  elements.statusSymbol.textContent = symbol;
  elements.statusTitle.textContent = title;
  elements.statusDetail.textContent = detail;
  elements.statusSymbol.classList.toggle("working", phase === "working");
}

function setProgress(percent) {
  elements.progressBar.style.width = `${percent}%`;
  elements.progressText.textContent = `${percent} %`;
  elements.progressTrack.setAttribute("aria-valuenow", String(percent));
}

function refreshState() {
  const ready = Boolean(
    webSerialSupported && state.port && state.firmware && !state.working,
  );
  elements.flashButton.disabled = !ready;
  elements.connectButton.disabled = !webSerialSupported || state.working;
  elements.bundledFirmware.disabled = state.working;
  for (const button of elements.boardGrid.querySelectorAll("button")) {
    button.disabled = state.working;
  }
  if (!state.working && state.phase !== "success" && state.phase !== "error") {
    setStatus(ready ? "ready" : "idle");
  }
}

function updateFirmwarePrompt() {
  elements.fileIcon.textContent = state.board.family === "esp32" ? "BIN" : "HEX";
  elements.fileInput.accept =
    state.board.family === "esp32"
      ? ".bin,application/octet-stream"
      : ".hex,text/plain";
  elements.releaseDownload.href = `../release/${state.board.releaseFile}`;
  elements.releaseDownload.download = state.board.releaseFile;
  if (!state.firmware) {
    elements.fileName.textContent = `${state.board.extension}-Datei hier ablegen`;
    elements.fileDetail.textContent = "oder die mitgelieferte Firmware verwenden";
  }
}

async function selectFirmware(file) {
  if (!file) return;
  if (!file.name.toLowerCase().endsWith(state.board.extension)) {
    state.firmware = null;
    setProgress(0);
    updateFirmwarePrompt();
    setStatus("error");
    log(`FEHLER: Für ${state.board.name} bitte eine ${state.board.extension}-Datei auswählen.`);
    refreshState();
    return;
  }

  try {
    const firmware =
      state.board.family === "esp32"
        ? parseEsp32FactoryBin(await file.arrayBuffer())
        : { ...parseIntelHex(await file.text()), kind: "avr" };
    if (firmware.highestAddress > state.board.maxFirmwareBytes) {
      throw new Error(
        `Firmware ist für ${state.board.name} zu groß (${firmware.highestAddress} Byte).`,
      );
    }
    state.firmware = firmware;
    state.phase = "idle";
    elements.fileName.textContent = file.name;
    elements.fileDetail.textContent =
      `${firmware.usedBytes.toLocaleString("de-DE")} Datenbytes · geprüft`;
    setProgress(0);
    log(`${file.name} geprüft: ${firmware.usedBytes.toLocaleString("de-DE")} Datenbytes.`);
  } catch (error) {
    state.firmware = null;
    updateFirmwarePrompt();
    setStatus("error");
    log(`FEHLER: ${error instanceof Error ? error.message : "Firmware ungültig."}`);
  }
  refreshState();
}

elements.boardGrid.addEventListener("click", (event) => {
  const button = event.target.closest("[data-board]");
  if (!button || state.working) return;
  const profile = BOARDS.find(
    (candidate) => candidate.id === button.dataset.board,
  );
  if (!profile) return;
  state.board = profile;
  state.phase = "idle";
  for (const candidate of elements.boardGrid.querySelectorAll("[data-board]")) {
    candidate.classList.toggle("selected", candidate === button);
  }
  log(`Board ausgewählt: ${profile.name}.`);
  if (
    state.firmware &&
    (state.firmware.kind !== profile.family ||
      state.firmware.highestAddress > profile.maxFirmwareBytes)
  ) {
    state.firmware = null;
    setProgress(0);
    log("Firmwareauswahl wurde an den neuen Boardtyp angepasst.");
  }
  updateFirmwarePrompt();
  refreshState();
});

elements.bundledFirmware.addEventListener("click", async () => {
  try {
    elements.bundledFirmware.disabled = true;
    log(`Lade ${state.board.releaseFile} …`);
    const response = await fetch(`../release/${state.board.releaseFile}`);
    if (!response.ok) throw new Error(`Download fehlgeschlagen (${response.status}).`);
    const file = new File([await response.blob()], state.board.releaseFile);
    await selectFirmware(file);
  } catch (error) {
    setStatus("error");
    log(`FEHLER: ${error instanceof Error ? error.message : "Firmware konnte nicht geladen werden."}`);
  } finally {
    elements.bundledFirmware.disabled = false;
  }
});

elements.fileInput.addEventListener("change", () => {
  void selectFirmware(elements.fileInput.files?.[0]);
  elements.fileInput.value = "";
});

for (const eventName of ["dragenter", "dragover"]) {
  elements.dropzone.addEventListener(eventName, (event) => {
    event.preventDefault();
    elements.dropzone.classList.add("dragging");
  });
}

for (const eventName of ["dragleave", "drop"]) {
  elements.dropzone.addEventListener(eventName, (event) => {
    event.preventDefault();
    elements.dropzone.classList.remove("dragging");
  });
}

elements.dropzone.addEventListener("drop", (event) => {
  void selectFirmware(event.dataTransfer?.files?.[0]);
});

elements.connectButton.addEventListener("click", async () => {
  try {
    state.port = await requestArduinoPort();
    state.phase = "idle";
    const description = describePort(state.port);
    elements.connectState.textContent = `Verbunden · ${description}`;
    elements.connectState.classList.add("connected");
    log(`USB-Gerät ausgewählt: ${description}.`);
  } catch (error) {
    const message =
      error instanceof DOMException && error.name === "NotFoundError"
        ? "Geräteauswahl abgebrochen."
        : error instanceof Error
          ? error.message
          : "USB-Gerät konnte nicht geöffnet werden.";
    log(message);
  }
  refreshState();
});

elements.flashButton.addEventListener("click", async () => {
  if (!state.port || !state.firmware) return;
  state.working = true;
  setStatus("working");
  setProgress(0);
  refreshState();
  elements.flashButton.textContent = "Firmware wird installiert …";
  log(`Starte Installation für ${state.board.name}.`);

  try {
    if (state.board.family === "esp32") {
      await flashEsp32Firmware({
        port: state.port,
        firmware: state.firmware,
        onLog: log,
        onProgress: setProgress,
      });
    } else {
      await flashFirmware({
        port: state.port,
        board: state.board,
        firmware: state.firmware,
        onLog: log,
        onProgress: setProgress,
      });
    }
    setStatus("success");
    log("FERTIG: Firmware erfolgreich installiert.");
  } catch (error) {
    setStatus("error");
    log(
      `FEHLER: ${error instanceof Error ? error.message : "Unbekannter Uploadfehler."}`,
    );
  } finally {
    state.port = null;
    state.working = false;
    elements.connectState.textContent = "Nach Upload getrennt";
    elements.connectState.classList.remove("connected");
    elements.flashButton.textContent = "Firmware installieren";
    refreshState();
  }
});

log("Web-Flasher bereit.");
log("Wähle Board, Firmware und USB-Gerät.");
updateFirmwarePrompt();
refreshState();
