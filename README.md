# Tactical Console

A self-contained tactical monitoring console for ESP32-S3 CAM deployment. The ESP32 serves the web UI directly from flash memory and streams live camera feed - all running offline with no cloud dependencies.

## What This Does

The ESP32-S3 CAM becomes a standalone web server that:

- Serves a tactical "cockpit" style web interface
- Streams live MJPEG video from the camera
- Provides REST API for device status and telemetry
- Runs entirely offline (no internet required)
- Creates its own WiFi network for direct connection

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      ESP32-S3 CAM                           │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              WebServer (port 80)                    │   │
│  │                                                     │   │
│  │  Static Files (Flash)       REST API               │   │
│  │  ├─ /           → HTML     ├─ /api/status          │   │
│  │  └─ /assets/    → JS       ├─ /api/telemetry       │   │
│  │                            ├─ /api/capture (JPEG)  │   │
│  │                            └─ /api/stream (MJPEG)  │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  Camera Module ──── OV2640/OV3660                           │
│  WiFi ─────────── AP Mode (creates "TacticalConsole")       │
└─────────────────────────────────────────────────────────────┘
         │
         │ WiFi Connection
         ▼
┌─────────────────────┐
│  Client (Laptop)    │
│  Browser → http://192.168.4.1
│                     │
│  ┌───────────────┐  │
│  │ Status Panel  │  │
│  │ Telemetry     │  │
│  │ Camera Feed   │  │
│  └───────────────┘  │
└─────────────────────┘
```

---

## Step-by-Step Guide

### Prerequisites

**On your development machine:**

- [Bun](https://bun.sh) installed (`curl -fsSL https://bun.sh/install | bash`)
- [Arduino IDE 2.x](https://www.arduino.cc/en/software) installed
- Git (optional, for version control)

**Hardware:**

- XIAO ESP32S3 Sense (or compatible ESP32-S3 with camera)
- USB-C cable for programming
- Camera module (OV2640 typically included with Sense variant)

---

### Step 1: Clone and Install Dependencies

```bash
# Clone the repository (or download ZIP)
git clone <your-repo-url> tactical-console
cd tactical-console

# Install Node/Bun dependencies
bun install
```

This installs Svelte and TypeScript types needed for building.

---

### Step 2: Build the Frontend

```bash
bun run build
```

This compiles the Svelte frontend:

- `src/ui/*.svelte` → compiled to JavaScript
- Output: `dist/assets/main.js` (~18KB)
- Output: `dist/index.html` (HTML shell)

**What happens internally:**

1. Custom Svelte plugin in `build.ts` compiles `.svelte` files
2. TypeScript in `<script lang="ts">` tags is transpiled by Bun
3. CSS is injected into the JavaScript bundle
4. Everything is minified for small size

---

### Step 3: Generate ESP32 Assets

```bash
bun run build:esp32
```

This converts the built frontend into a C++ header file:

- Reads `dist/index.html` and `dist/assets/main.js`
- Embeds them as `PROGMEM` strings in `esp32/web_assets.h`
- These get stored in ESP32 flash memory (not RAM)

**Output file structure:**

```cpp
// esp32/web_assets.h (auto-generated)
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>...)rawliteral";
const char MAIN_JS[] PROGMEM = R"rawliteral(function...)rawliteral";
```

---

### Step 4: Set Up Arduino IDE

#### 4.1 Install ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. In "Additional boards manager URLs", add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32" and install **esp32 by Espressif Systems**

#### 4.2 Select Board Settings

Go to **Tools** menu and configure:

| Setting          | Value                                |
| ---------------- | ------------------------------------ |
| Board            | XIAO_ESP32S3                         |
| USB CDC On Boot  | Enabled                              |
| CPU Frequency    | 240MHz (WiFi)                        |
| Flash Mode       | QIO 80MHz                            |
| Flash Size       | 8MB (or your board's size)           |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| PSRAM            | **OPI PSRAM**                        |
| Upload Speed     | 921600                               |

**Important:** The "Huge APP" partition is required to fit the web assets. PSRAM enables higher resolution camera frames.

---

### Step 5: Configure WiFi (Optional)

Open `esp32/tactical_console.ino` and edit the WiFi settings:

```cpp
// AP Mode (default) - ESP32 creates its own network
const bool AP_MODE = true;
const char* ap_ssid = "TacticalConsole";     // Network name
const char* ap_password = "tactical123";      // Password (min 8 chars)

// Station Mode - ESP32 connects to existing network
const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";
```

**AP Mode (recommended for demos):** ESP32 creates a WiFi hotspot. Connect your laptop/phone directly to it.

**Station Mode:** ESP32 joins your existing WiFi network. Check Serial Monitor for assigned IP.

---

### Step 6: Upload to ESP32

1. Connect ESP32-S3 to your computer via USB-C

2. In Arduino IDE, select the correct port:
   **Tools → Port → /dev/cu.usbmodem\* (macOS)** or **COM\* (Windows)**

3. Open the sketch:
   **File → Open → `esp32/tactical_console.ino`**

4. Click **Upload** (→ arrow button)

5. Wait for compilation and upload to complete (~1-2 minutes)

**Expected output:**

```
Sketch uses 1,234,567 bytes (X%) of program storage space.
Global variables use 123,456 bytes (Y%) of dynamic memory.
...
Hard resetting via RTS pin...
```

---

### Step 7: Connect and Test

#### If using AP Mode (default):

1. On your laptop/phone, open WiFi settings
2. Connect to network: **TacticalConsole**
3. Password: **tactical123**
4. Open browser and go to: **http://192.168.4.1**

#### If using Station Mode:

1. Open Arduino IDE Serial Monitor (**Tools → Serial Monitor**)
2. Set baud rate to **115200**
3. Press reset button on ESP32 or reconnect USB
4. Look for the IP address in output:
   ```
   [tactical-console] Connected - IP: 192.168.1.XXX
   ```
5. Open browser and go to that IP address

---

### Step 8: Using the Interface

The tactical console shows:

| Panel         | Description                                          |
| ------------- | ---------------------------------------------------- |
| **STATUS**    | Device ID, uptime, firmware version, recording state |
| **TELEMETRY** | GPS coordinates, speed, heading (mock data)          |
| **CAMERA**    | Live video feed with STREAM/SNAP controls            |

**Camera Controls:**

- **STREAM** - Start live MJPEG video stream
- **STOP** - Stop the stream
- **SNAP** - Capture single JPEG frame

---

## Project Structure

```
tactical-console/
├── esp32/                          # Arduino sketch (runs on device)
│   ├── tactical_console.ino        # Main sketch
│   ├── camera_pins.h               # GPIO definitions
│   └── web_assets.h                # Generated frontend (PROGMEM)
│
├── src/
│   ├── server/                     # Bun dev server (for testing)
│   │   ├── index.ts
│   │   └── routes/
│   │       ├── status.ts
│   │       └── telemetry.ts
│   └── ui/                         # Svelte frontend
│       ├── App.svelte
│       ├── main.ts
│       └── components/
│           ├── StatusPanel.svelte
│           ├── TelemetryPanel.svelte
│           └── CameraFeed.svelte
│
├── static/
│   └── index.html                  # HTML template
│
├── scripts/
│   └── generate-esp32-assets.ts    # Converts dist/ to C++ header
│
├── tests/                          # Test suite
│   ├── api.test.ts
│   ├── build.test.ts
│   └── esp32.test.ts
│
├── build.ts                        # Custom Svelte bundler for Bun
├── package.json
└── README.md
```

---

## Development Workflow

For rapid iteration, use the Bun dev server:

```bash
# Start dev server with hot reload
bun run dev

# Open http://localhost:3000
```

The Bun server mirrors the ESP32's API endpoints, so you can develop the frontend without uploading to hardware each time.

**Workflow:**

1. Edit Svelte components in `src/ui/`
2. Test in browser with `bun run dev`
3. When ready, build for ESP32: `bun run build:esp32`
4. Upload to device in Arduino IDE

---

## API Reference

| Endpoint             | Method | Description               |
| -------------------- | ------ | ------------------------- |
| `/`                  | GET    | Serves index.html         |
| `/assets/main.js`    | GET    | Serves compiled frontend  |
| `/api/status`        | GET    | Device status JSON        |
| `/api/telemetry`     | GET    | GPS/motion telemetry JSON |
| `/api/capture`       | GET    | Single JPEG frame         |
| `/api/stream`        | GET    | MJPEG video stream        |
| `/api/record/toggle` | POST   | Toggle recording state    |

### GET /api/status

```json
{
  "deviceId": "ESP32-S3-CAM",
  "uptimeSeconds": 12345,
  "version": "1.0.0",
  "recording": false
}
```

### GET /api/telemetry

```json
{
  "lat": 33.4484,
  "lon": -112.074,
  "speedKph": 72,
  "headingDeg": 135
}
```

---

## Customization

### Adding Real GPS

Replace mock telemetry in `tactical_console.ino`:

```cpp
#include <TinyGPS++.h>
TinyGPSPlus gps;
HardwareSerial GPS_Serial(1);

void setup() {
  GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

void updateTelemetry() {
  while (GPS_Serial.available()) {
    gps.encode(GPS_Serial.read());
  }
  if (gps.location.isValid()) {
    mockLat = gps.location.lat();
    mockLon = gps.location.lng();
    mockSpeed = gps.speed.kmph();
    mockHeading = gps.course.deg();
  }
}
```

### Changing Camera Resolution

In `initCamera()` function:

```cpp
// Options: FRAMESIZE_QVGA (320x240), FRAMESIZE_VGA (640x480),
//          FRAMESIZE_SVGA (800x600), FRAMESIZE_XGA (1024x768)
config.frame_size = FRAMESIZE_VGA;
```

Higher resolutions need more memory and reduce frame rate.

### Different ESP32-S3 Board

Edit `esp32/camera_pins.h` with your board's GPIO mapping. Common boards:

- XIAO ESP32S3 Sense (default)
- AI-Thinker ESP32-CAM (ESP32, not S3)
- Freenove ESP32-S3-WROOM CAM

---

## Running Tests

```bash
# Run all tests
bun test

# Run specific test file
bun test:api      # API endpoint tests
bun test:build    # Build process tests
bun test:esp32    # ESP32 asset generation tests
```

---

## Troubleshooting

### "Failed to connect to ESP32"

- Hold BOOT button while clicking Upload
- Try different USB cable (data-capable, not charge-only)
- Check correct port is selected

### Camera shows "Stream unavailable"

- Ensure camera ribbon cable is firmly seated
- Check camera_pins.h matches your board
- Try reducing resolution in initCamera()

### Web page doesn't load

- Verify WiFi connection to TacticalConsole network
- Check IP address (192.168.4.1 for AP mode)
- Open Serial Monitor to see any errors

### "Sketch too big"

- Select **Huge APP (3MB No OTA)** partition scheme
- Ensure web_assets.h isn't accidentally duplicated

---

## Flash Memory Usage

| Component            | Size        |
| -------------------- | ----------- |
| Arduino sketch       | ~800 KB     |
| Web assets (HTML+JS) | ~18 KB      |
| Camera library       | ~200 KB     |
| WiFi library         | ~150 KB     |
| **Total**            | **~1.2 MB** |

With 3MB APP partition, there's plenty of room for additional features.

---

## License

MIT - Use freely for your projects.
