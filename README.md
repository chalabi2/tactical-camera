# Tactical Console

A self-contained tactical monitoring console for ESP32-S3 CAM with IMU orientation sensing. The device serves a web UI directly from flash memory with live camera streaming and real-time orientation data - all running offline.

## Gallery

<table>
  <tr>
    <td width="50%" align="center">
      <strong>Camera Feed</strong><br>
      <img src="./image/camera.PNG" width="100%">
    </td>
    <td width="50%" align="center">
      <strong>IMU Orientation</strong><br>
      <img src="./image/gyro.PNG" width="100%">
    </td>
  </tr>
</table>

---

## Features

- **Live Camera Feed** - MJPEG streaming from OV2640/OV3660 camera
- **IMU Orientation** - Real-time pitch/roll from MPU-6050 gyroscope/accelerometer
- **Self-Hosted Web UI** - Served directly from ESP32 flash memory
- **Offline Operation** - Creates its own WiFi network, no internet required
- **REST API** - Status and telemetry endpoints

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
2. Open browser: **http://tactical.local** (or `http://192.168.4.1`)

---

## Hardware

### Required Components

| Component           | Description                    | Connection |
| ------------------- | ------------------------------ | ---------- |
| XIAO ESP32-S3 Sense | Main board with camera         | -          |
| MPU-6050            | 6-axis gyroscope/accelerometer | I2C        |

### MPU-6050 Wiring

| MPU-6050 Pin | ESP32-S3 Pin                   |
| ------------ | ------------------------------ |
| VCC          | 3V3                            |
| GND          | GND                            |
| SCL          | D5 (GPIO6)                     |
| SDA          | D4 (GPIO5)                     |
| ADO          | GND (sets I2C address to 0x68) |
| INT          | Not connected                  |
| XDA/XCL      | Not connected                  |

---

## Architecture

```
ESP32-S3 CAM + MPU-6050
├── WebServer (port 80)
│   ├── /              → Web UI (from flash)
│   ├── /api/status    → Device status
│   ├── /api/telemetry → IMU orientation data
│   ├── /api/capture   → JPEG snapshot
│   └── /api/stream    → MJPEG live feed
├── I2C Bus
│   └── MPU-6050 → pitch, roll, accel, gyro
└── WiFi AP → "TacticalConsole"
```

---

## Project Structure

```
tactical-console/
├── tactical_console/         # Arduino sketch
│   ├── tactical_console.ino  # Main sketch (camera + IMU)
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

### Different Camera Board

Edit `tactical_console/camera_pins.h` with your board's GPIO mapping.

### IMU Configuration

The MPU-6050 is configured for:

- Accelerometer: +/- 2g range
- Gyroscope: +/- 250 deg/s range
- Update rate: 20Hz

Modify `initIMU()` and `readIMU()` in `tactical_console.ino` to adjust sensitivity or add filtering.

---

## Build Output

| Component             | File                  | Size        |
| --------------------- | --------------------- | ----------- |
| Frontend              | `dist/index.html`     | 545 B       |
| Frontend              | `dist/assets/main.js` | 30.174 KB   |
| **Frontend Total**    |                       | **30.7 KB** |
| Backend (dev server)  | `build/index.js`      | 2.1 KB      |
| ESP32 sketch + assets | `.bin`                | ~1.2 MB     |

---

## Disclaimer

Portions of this codebase were generated with assistance from a private LLM hosted and operated locally (qwen 3 coder 30b model on LM Studio). All generated code has been reviewed, tested, and modified as needed to ensure correctness and fitness for purpose.
