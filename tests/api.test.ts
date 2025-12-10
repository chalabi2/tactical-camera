import { describe, expect, test, beforeAll, afterAll } from "bun:test";

const PORT = 9999;
let serverProcess: Bun.Subprocess | null = null;

describe("API Endpoints", () => {
  beforeAll(async () => {
    // Start server for testing
    serverProcess = Bun.spawn(["bun", "src/server/index.ts"], {
      env: { ...process.env, PORT: String(PORT) },
      stdout: "pipe",
      stderr: "pipe",
    });
    // Wait for server to start
    await new Promise((resolve) => setTimeout(resolve, 500));
  });

  afterAll(() => {
    try {
      serverProcess?.kill();
    } catch {
      // Process may already be terminated or permission denied in sandbox
    }
  });

  test("GET /api/status returns valid status object", async () => {
    const res = await fetch(`http://localhost:${PORT}/api/status`);
    expect(res.status).toBe(200);
    expect(res.headers.get("content-type")).toContain("application/json");

    const data = await res.json();
    expect(data).toHaveProperty("deviceId");
    expect(data).toHaveProperty("uptimeSeconds");
    expect(data).toHaveProperty("version");
    expect(data).toHaveProperty("recording");

    expect(typeof data.deviceId).toBe("string");
    expect(typeof data.uptimeSeconds).toBe("number");
    expect(typeof data.version).toBe("string");
    expect(typeof data.recording).toBe("boolean");
  });

  test("GET /api/telemetry returns valid telemetry object", async () => {
    const res = await fetch(`http://localhost:${PORT}/api/telemetry`);
    expect(res.status).toBe(200);
    expect(res.headers.get("content-type")).toContain("application/json");

    const data = await res.json();
    expect(data).toHaveProperty("lat");
    expect(data).toHaveProperty("lon");
    expect(data).toHaveProperty("speedKph");
    expect(data).toHaveProperty("headingDeg");

    expect(typeof data.lat).toBe("number");
    expect(typeof data.lon).toBe("number");
    expect(typeof data.speedKph).toBe("number");
    expect(typeof data.headingDeg).toBe("number");

    // Validate coordinate ranges
    expect(data.lat).toBeGreaterThanOrEqual(-90);
    expect(data.lat).toBeLessThanOrEqual(90);
    expect(data.lon).toBeGreaterThanOrEqual(-180);
    expect(data.lon).toBeLessThanOrEqual(180);

    // Validate heading range
    expect(data.headingDeg).toBeGreaterThanOrEqual(0);
    expect(data.headingDeg).toBeLessThan(360);
  });

  test("GET /api/status uptime increases over time", async () => {
    const res1 = await fetch(`http://localhost:${PORT}/api/status`);
    const data1 = await res1.json();

    await new Promise((resolve) => setTimeout(resolve, 1100));

    const res2 = await fetch(`http://localhost:${PORT}/api/status`);
    const data2 = await res2.json();

    expect(data2.uptimeSeconds).toBeGreaterThan(data1.uptimeSeconds);
  });

  test("GET / returns HTML", async () => {
    const res = await fetch(`http://localhost:${PORT}/`);
    expect(res.status).toBe(200);
    expect(res.headers.get("content-type")).toContain("text/html");

    const html = await res.text();
    expect(html).toContain("<!DOCTYPE html>");
    expect(html).toContain("Tactical Console");
  });

  test("GET /nonexistent returns 404", async () => {
    const res = await fetch(`http://localhost:${PORT}/nonexistent`);
    expect(res.status).toBe(404);
  });

  test("API responses have no-cache header", async () => {
    const statusRes = await fetch(`http://localhost:${PORT}/api/status`);
    expect(statusRes.headers.get("cache-control")).toBe("no-cache");

    const telemetryRes = await fetch(`http://localhost:${PORT}/api/telemetry`);
    expect(telemetryRes.headers.get("cache-control")).toBe("no-cache");
  });
});

