import { describe, expect, test, beforeAll } from "bun:test";

describe("ESP32 Asset Generation", () => {
  beforeAll(async () => {
    // Ensure esp32 assets have been generated
    const proc = Bun.spawn(["bun", "run", "build:esp32"], {
      cwd: import.meta.dir + "/..",
      stdout: "pipe",
      stderr: "pipe",
    });
    await proc.exited;
  });

  test("web_assets.h exists", async () => {
    const file = Bun.file("esp32/web_assets.h");
    expect(await file.exists()).toBe(true);
  });

  test("web_assets.h contains INDEX_HTML", async () => {
    const content = await Bun.file("esp32/web_assets.h").text();
    expect(content).toContain("INDEX_HTML");
    expect(content).toContain("PROGMEM");
  });

  test("web_assets.h contains MAIN_JS", async () => {
    const content = await Bun.file("esp32/web_assets.h").text();
    expect(content).toContain("MAIN_JS");
  });

  test("web_assets.h is valid C++ (no syntax breakers)", async () => {
    const content = await Bun.file("esp32/web_assets.h").text();

    // Should not contain the raw literal delimiter inside content
    const afterFirstDelim = content.split('R"rawliteral(')[1];
    if (afterFirstDelim) {
      const contentPart = afterFirstDelim.split(')rawliteral"')[0];
      expect(contentPart).not.toContain(')rawliteral"');
    }
  });

  test("web_assets.h total size is reasonable for ESP32 (< 100KB)", async () => {
    const file = Bun.file("esp32/web_assets.h");
    expect(file.size).toBeLessThan(100 * 1024);
  });
});

describe("ESP32 Arduino Sketch", () => {
  test("tactical_console.ino exists", async () => {
    const file = Bun.file("esp32/tactical_console.ino");
    expect(await file.exists()).toBe(true);
  });

  test("camera_pins.h exists", async () => {
    const file = Bun.file("esp32/camera_pins.h");
    expect(await file.exists()).toBe(true);
  });

  test("sketch includes web_assets.h", async () => {
    const content = await Bun.file("esp32/tactical_console.ino").text();
    expect(content).toContain('#include "web_assets.h"');
  });

  test("sketch includes camera_pins.h", async () => {
    const content = await Bun.file("esp32/tactical_console.ino").text();
    expect(content).toContain('#include "camera_pins.h"');
  });

  test("sketch defines API endpoints", async () => {
    const content = await Bun.file("esp32/tactical_console.ino").text();
    expect(content).toContain("/api/status");
    expect(content).toContain("/api/telemetry");
    expect(content).toContain("/api/capture");
    expect(content).toContain("/api/stream");
  });

  test("sketch has AP mode configuration", async () => {
    const content = await Bun.file("esp32/tactical_console.ino").text();
    expect(content).toContain("AP_MODE");
    expect(content).toContain("softAP");
  });

  test("sketch has camera init function", async () => {
    const content = await Bun.file("esp32/tactical_console.ino").text();
    expect(content).toContain("initCamera");
    expect(content).toContain("esp_camera_init");
  });

  test("sketch has MJPEG streaming handler", async () => {
    const content = await Bun.file("esp32/tactical_console.ino").text();
    expect(content).toContain("handleStream");
    expect(content).toContain("multipart/x-mixed-replace");
  });

  test("camera_pins.h defines XIAO ESP32S3 pins", async () => {
    const content = await Bun.file("esp32/camera_pins.h").text();
    expect(content).toContain("CAMERA_MODEL_XIAO_ESP32S3");
    expect(content).toContain("Y2_GPIO_NUM");
    expect(content).toContain("VSYNC_GPIO_NUM");
  });
});

