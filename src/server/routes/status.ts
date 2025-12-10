/**
 * Status API Route
 * 
 * Returns device status information including:
 * - Device ID (hardware identifier)
 * - Uptime in seconds
 * - Firmware version
 * - Recording state (camera active/inactive)
 * 
 * On real ESP32-S3 hardware, replace mock values with:
 * - Device ID from flash/EEPROM
 * - System uptime from OS
 * - Recording state from camera service
 */

import { serverStartTime } from "../index";

interface StatusResponse {
  deviceId: string;
  uptimeSeconds: number;
  version: string;
  recording: boolean;
}

// Simulated recording state - toggles every 30 seconds for demo purposes
function getRecordingState(): boolean {
  const elapsed = Math.floor((Date.now() - serverStartTime) / 1000);
  return Math.floor(elapsed / 30) % 2 === 0;
}

export function handleStatus(_req: Request): Response {
  const uptimeSeconds = Math.floor((Date.now() - serverStartTime) / 1000);
  
  // TODO: Replace with actual sensor/system reads on ESP32-S3
  // Example integration points:
  // - Read device ID from: /sys/class/dmi/id/product_serial or flash storage
  // - Read uptime from: /proc/uptime or system call
  // - Read recording state from: Unix socket to camera service (e.g., /var/run/camera.sock)
  
  const status: StatusResponse = {
    deviceId: "ESP32-CAM-001",
    uptimeSeconds,
    version: "1.0.0",
    recording: getRecordingState(),
  };

  return Response.json(status, {
    headers: {
      "Cache-Control": "no-cache",
    },
  });
}

