/**
 * Telemetry API Route
 * 
 * Returns real-time telemetry data:
 * - GPS coordinates (latitude/longitude)
 * - Speed in km/h
 * - Heading in degrees (0-360)
 * 
 * On real ESP32-S3 hardware, replace mock values with:
 * - GPS data from UART/I2C GPS module
 * - Speed calculated from GPS delta or wheel sensor
 * - Heading from magnetometer or GPS course
 */

import { serverStartTime } from "../index";

interface TelemetryResponse {
  lat: number;
  lon: number;
  speedKph: number;
  headingDeg: number;
  timestamp: number;
}

// Simulated GPS coordinates - oscillates around a base position
// Base: Phoenix, AZ area (matching the spec example)
const BASE_LAT = 33.4484;
const BASE_LON = -112.0740;

function getMockTelemetry(): TelemetryResponse {
  const elapsed = (Date.now() - serverStartTime) / 1000;
  
  // Simulate slight movement for demo - small oscillation pattern
  const latOffset = Math.sin(elapsed / 10) * 0.001;
  const lonOffset = Math.cos(elapsed / 10) * 0.001;
  
  // Speed varies between 0-120 kph
  const speedKph = Math.abs(Math.sin(elapsed / 5) * 80) + 20;
  
  // Heading rotates slowly (0-360 degrees)
  const headingDeg = (elapsed * 2) % 360;
  
  return {
    lat: BASE_LAT + latOffset,
    lon: BASE_LON + lonOffset,
    speedKph: Math.round(speedKph),
    headingDeg: Math.round(headingDeg),
    timestamp: Date.now(),
  };
}

export function handleTelemetry(_req: Request): Response {
  // TODO: Replace with actual sensor reads on ESP32-S3
  // Example integration points:
  // - GPS via UART: Read NMEA sentences from /dev/ttyUSB0 or similar
  // - GPS via I2C: Query GPS module directly using Bun's FFI
  // - IPC option: Read from shared memory or Unix socket if separate GPS daemon
  //
  // For ESP32-S3 specifically:
  // - Use Bun FFI to call ESP-IDF GPS driver functions
  // - Or read from a pre-processed JSON file updated by a C service
  
  const telemetry = getMockTelemetry();

  return Response.json(telemetry, {
    headers: {
      "Cache-Control": "no-cache",
    },
  });
}

