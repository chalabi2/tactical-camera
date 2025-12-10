# Tactical Console

A self-contained tactical monitoring console for ESP32-S3 CAM. The device serves a web UI directly from flash memory with live camera streaming - all running offline.

## Demo

<!-- Add GIF or video of the hardware device and web UI here -->
<!-- ![Tactical Console Demo](./assets/demo.gif) -->

_Demo video/GIF coming soon - shows ESP32-S3 hardware and live web interface_

---

## Features

- **Live Camera Feed** - MJPEG streaming from OV2640/OV3660 camera (main feature)
- **Self-Hosted Web UI** - Served directly from ESP32 flash memory
- **Offline Operation** - Creates its own WiFi network, no internet required
- **REST API** - Status and telemetry endpoints

> **Note:** Status and telemetry data are **mock/placeholder values**. The primary feature is the camera streaming. Replace mock data with real sensors.

---

## Quick Start

### Dependencies
1. bun
2. arduino-cli

### 1. Build the Frontend

```bash
bun install
bun run build:esp32
```

### 2. Upload to ESP32

1. Compile

```bash
arduino-cli compile --fqbn esp32:esp32:<your esp32 model> tactical_console/
```
2. Upload

```bash
arduino-cli upload --fqbn esp32:esp32:XIAO_ESP32S3 -p (arduino-cli board list | awk '/t
ty/ {print $1; exit}') tactical_console/
```


### 3. Connect

1. Join WiFi network: **TacticalConsole** (password: `tactical123`)
2. Open browser: **http://192.168.4.1**

---

## Architecture

```
ESP32-S3 CAM
├── WebServer (port 80)
│   ├── /              → Web UI (from flash)
│   ├── /api/status    → Device status (mock)
│   ├── /api/telemetry → GPS/heading (mock)
│   ├── /api/capture   → JPEG snapshot
│   └── /api/stream    → MJPEG live feed
└── WiFi AP → "TacticalConsole"
```

---

## Project Structure

```
tactical-console/
├── esp32/                    # Arduino sketch
│   ├── tactical_console.ino  # Main sketch
│   ├── camera_pins.h         # GPIO config
│   └── web_assets.h          # Generated (bun run build:esp32)
├── src/
│   ├── server/               # Bun dev server
│   └── ui/                   # Svelte frontend
├── build.ts                  # Custom Svelte bundler
└── package.json
```

---

## Development

```bash
# Dev server (mirrors ESP32 API)
bun run dev

# Build frontend only
bun run build

# Build + generate ESP32 assets
bun run build:esp32

# Run tests
bun test
```

---

## Customization

### Adding Real GPS

Replace mock values in `tactical_console.ino`:

```cpp
#include <TinyGPS++.h>
TinyGPSPlus gps;

void updateTelemetry() {
  while (GPS_Serial.available()) {
    gps.encode(GPS_Serial.read());
  }
  if (gps.location.isValid()) {
    mockLat = gps.location.lat();
    mockLon = gps.location.lng();
  }
}
```

### Different Camera Board

Edit `esp32/camera_pins.h` with your board's GPIO mapping.

---

## Build Output

| File                    | Size    |
| ----------------------- | ------- |
| Frontend (HTML+JS)      | ~21 KB  |
| Arduino sketch + assets | ~1.2 MB |

---

## License

MIT
