# Tactical Console

A self-contained tactical monitoring console for ESP32-S3 CAM deployment. Runs entirely on the device with no cloud dependencies.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      ESP32-S3 CAM                           │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              WebServer (port 80)                    │   │
│  │                                                     │   │
│  │  Static Files (PROGMEM)     REST API               │   │
│  │  ├─ /           → HTML     ├─ /api/status          │   │
│  │  └─ /assets/    → JS       ├─ /api/telemetry       │   │
│  │                            ├─ /api/capture         │   │
│  │                            └─ /api/stream          │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  Camera ──┬── JPEG capture                                  │
│           └── MJPEG stream                                  │
│                                                             │
│  WiFi AP Mode ─── Connect to "TacticalConsole"              │
└─────────────────────────────────────────────────────────────┘
```

**Key points:**

- ESP32-S3 runs the web server directly (Arduino/ESP-IDF)
- Frontend is compiled to JS and embedded in flash as PROGMEM
- No external dependencies - everything runs offline
- Bun server is for development only

## Project Structure

```
tactical-console/
├── esp32/                    # Arduino sketch for ESP32-S3
│   ├── tactical_console.ino  # Main sketch with web server
│   ├── camera_pins.h         # Camera GPIO definitions
│   └── web_assets.h          # Generated frontend (PROGMEM)
├── src/
│   ├── server/               # Bun dev server (mirrors ESP32 API)
│   │   ├── index.ts          # HTTP server
│   │   └── routes/           # API handlers
│   └── ui/                   # Svelte frontend
│       ├── App.svelte        # Main layout
│       ├── main.ts           # Entry point
│       └── components/       # UI panels
├── static/
│   └── index.html            # HTML shell
├── scripts/
│   └── generate-esp32-assets.ts  # Converts dist/ to C++ header
├── dist/                     # Built frontend (generated)
├── build/                    # Built Bun server (generated)
├── build.ts                  # Custom Svelte bundler for Bun
├── package.json
└── README.md
```

## Development Workflow

### 1. Install Dependencies

```bash
bun install
```

### 2. Build Frontend

```bash
bun run build
```

This runs `build.ts` which:

1. Compiles Svelte components using a custom Bun plugin
2. Bundles to `dist/assets/main.js`
3. Copies `static/index.html` to `dist/`

### 3. Run Dev Server

```bash
bun run dev
```

Opens at http://localhost:3000 (or set PORT env var).

### 4. Generate ESP32 Assets

```bash
bun run build:esp32
```

This builds the frontend and generates `esp32/web_assets.h` containing the HTML and JS as PROGMEM strings.

## Custom Svelte Bundler

Bun doesn't natively compile Svelte, and the `esbuild-svelte` plugin uses `onEnd` callbacks which Bun doesn't support. The solution is a custom plugin in `build.ts`:

```typescript
const sveltePlugin: BunPlugin = {
  name: "svelte",
  setup(build) {
    build.onLoad({ filter: /\.svelte$/ }, async (args) => {
      const source = await Bun.file(args.path).text();

      // Preprocess TypeScript in script tags
      const preprocessed = await preprocess(source, tsPreprocessor, {
        filename: args.path,
      });

      // Compile Svelte to JS
      const result = compile(preprocessed.code, {
        filename: args.path,
        generate: "dom",
        css: "injected", // CSS embedded in JS
        dev: false,
      });

      return { contents: result.js.code, loader: "js" };
    });
  },
};
```

The TypeScript preprocessor uses Bun's transpiler:

```typescript
const tsPreprocessor = {
  script: async ({ content, attributes }) => {
    if (attributes.lang !== "ts") return { code: content };

    const transpiler = new Bun.Transpiler({
      loader: "ts",
      target: "browser",
    });
    return { code: transpiler.transformSync(content) };
  },
};
```

## ESP32 Deployment

### Hardware Requirements

- XIAO ESP32S3 Sense (or compatible ESP32-S3 CAM board)
- OV2640 or OV3660 camera module
- PSRAM recommended for higher resolution

### Arduino IDE Setup

1. Install ESP32 board support in Arduino IDE
2. Select board: **XIAO_ESP32S3**
3. Set partition scheme: **Huge APP (3MB No OTA/1MB SPIFFS)**
4. Enable PSRAM: **OPI PSRAM**

### Build & Upload

1. Generate the web assets:

   ```bash
   bun run build:esp32
   ```

2. Open `esp32/tactical_console.ino` in Arduino IDE

3. Edit WiFi credentials in the sketch (or use AP mode)

4. Upload to ESP32-S3

### Connecting

**AP Mode (default):**

- ESP32 creates WiFi network "TacticalConsole"
- Password: `tactical123`
- Connect and browse to `http://192.168.4.1`

**Station Mode:**

- Set `AP_MODE = false` in sketch
- Enter your WiFi credentials
- ESP32 prints IP to serial monitor

## API Endpoints

### GET /api/status

Device status information.

```json
{
  "deviceId": "ESP32-S3-CAM",
  "uptimeSeconds": 12345,
  "version": "1.0.0",
  "recording": true
}
```

### GET /api/telemetry

GPS/motion telemetry (mock data - replace with real sensors).

```json
{
  "lat": 33.4484,
  "lon": -112.074,
  "speedKph": 72,
  "headingDeg": 135
}
```

### GET /api/capture

Returns a single JPEG frame from the camera.

### GET /api/stream

Returns MJPEG stream (multipart/x-mixed-replace).

### POST /api/record/toggle

Toggles recording state. Returns:

```json
{
  "recording": true
}
```

## Sensor Integration

The mock telemetry in `updateMockTelemetry()` should be replaced with real sensor reads:

```cpp
// GPS via UART
#include <TinyGPS++.h>
TinyGPSPlus gps;

void updateRealTelemetry() {
  while (Serial1.available()) {
    gps.encode(Serial1.read());
  }
  if (gps.location.isValid()) {
    mockLat = gps.location.lat();
    mockLon = gps.location.lng();
    mockSpeed = gps.speed.kmph();
    mockHeading = gps.course.deg();
  }
}
```

```cpp
// IMU via I2C (for heading when stationary)
#include <Adafruit_LSM6DS3TRC.h>
Adafruit_LSM6DS3TRC imu;

void updateIMU() {
  sensors_event_t accel, gyro, temp;
  imu.getEvent(&accel, &gyro, &temp);
  // Calculate heading from accelerometer/gyro
}
```

## Offline / Air-Gapped Design

This project is designed for environments with:

- **No internet** - All assets served from ESP32 flash
- **No cloud** - No external API calls
- **No CDNs** - Fonts and styles embedded in bundle
- **No analytics** - No telemetry sent externally

The entire application runs on a single ESP32-S3 with:

- ~25KB frontend bundle (HTML + JS + CSS)
- ~200KB Arduino sketch
- WiFi AP for direct device connection

## File Sizes

After `bun run build:esp32`:

| File                | Size       |
| ------------------- | ---------- |
| dist/index.html     | ~350 bytes |
| dist/assets/main.js | ~15 KB     |
| esp32/web_assets.h  | ~16 KB     |

Total flash usage for web assets: **~16 KB**

## License

MIT
