/**
 * Telemetry API Route
 * 
 * Returns real-time telemetry data from sensors:
 * - IMU orientation (pitch, roll) from MPU-6050
 * - Acceleration (g-force) from MPU-6050
 * - Angular velocity (deg/s) from MPU-6050
 * - Internal temperature from MPU-6050
 * 
 * Hardware: MPU-6050 connected via I2C (SDA: D4/GPIO5, SCL: D5/GPIO6)
 * 
 * Dev server returns simulated values that mirror the ESP32 response format.
 */

import { serverStartTime } from "../index";

interface AccelData {
  x: number;
  y: number;
  z: number;
}

interface GyroData {
  x: number;
  y: number;
  z: number;
}

interface OrientationData {
  available: boolean;
  pitch: number;
  roll: number;
  accel: AccelData;
  gyro: GyroData;
  temp: number;
}

interface TelemetryResponse {
  orientation: OrientationData;
  timestamp: number;
}

function getDevTelemetry(): TelemetryResponse {
  const elapsed = (Date.now() - serverStartTime) / 1000;
  
  const noise = () => (Math.random() - 0.5) * 0.5;
  const pitch = Math.sin(elapsed / 3) * 15 + noise();
  const roll = Math.cos(elapsed / 4) * 10 + noise();
  
  const pitchRad = pitch * Math.PI / 180;
  const rollRad = roll * Math.PI / 180;
  
  return {
    orientation: {
      available: true,
      pitch: Math.round(pitch * 100) / 100,
      roll: Math.round(roll * 100) / 100,
      accel: {
        x: Math.round(Math.sin(pitchRad) * 1000) / 1000,
        y: Math.round(Math.sin(rollRad) * Math.cos(pitchRad) * 1000) / 1000,
        z: Math.round(Math.cos(rollRad) * Math.cos(pitchRad) * 1000) / 1000,
      },
      gyro: {
        x: Math.round((Math.cos(elapsed / 3) * 5 + noise() * 2) * 100) / 100,
        y: Math.round((-Math.sin(elapsed / 4) * 2.5 + noise() * 2) * 100) / 100,
        z: Math.round(noise() * 3 * 100) / 100,
      },
      temp: Math.round((25 + Math.sin(elapsed / 60) * 2) * 10) / 10,
    },
    timestamp: Date.now(),
  };
}

export function handleTelemetry(_req: Request): Response {
  const telemetry = getDevTelemetry();

  return Response.json(telemetry, {
    headers: {
      "Cache-Control": "no-cache",
    },
  });
}

