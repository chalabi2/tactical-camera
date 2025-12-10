/**
 * Tactical Console - ESP32-S3 CAM
 * 
 * Serves the tactical console web UI and REST API directly from the ESP32.
 * Combines camera streaming with status/telemetry endpoints.
 * 
 * Build requirements:
 * - Board: XIAO ESP32S3 Sense (or your ESP32-S3 CAM variant)
 * - Partition: Huge APP (3MB No OTA)
 * - PSRAM: OPI PSRAM
 * 
 * Hardware connections:
 * - MPU-6050: SDA->D4(GPIO5), SCL->D5(GPIO6), VCC->3.3V, GND->GND
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include "web_assets.h"  // Generated from frontend build

#define CAMERA_MODEL_XIAO_ESP32S3
#include "camera_pins.h"

// MPU-6050 I2C address (ADO pin to GND = 0x68, to VCC = 0x69)
#define MPU6050_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H 0x43

// I2C pins for MPU-6050 (separate from camera I2C)
#define IMU_SDA_PIN 5   // D4
#define IMU_SCL_PIN 6   // D5

// WiFi credentials - change these for your network
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Set to true for AP mode (device creates its own network)
const bool AP_MODE = true;
const char* ap_ssid = "TacticalConsole";
const char* ap_password = "tactical123";

// mDNS hostname - access via http://tactical.local
const char* mdns_hostname = "tactical";

WebServer server(80);

// Device state
unsigned long startTime;
bool isRecording = false;

// IMU state (MPU-6050)
bool imuAvailable = false;
float accelX = 0, accelY = 0, accelZ = 0;  // Acceleration in g
float gyroX = 0, gyroY = 0, gyroZ = 0;      // Angular velocity in deg/s
float pitch = 0, roll = 0;                  // Calculated orientation angles
float temperature = 0;                       // MPU-6050 internal temp sensor

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[tactical-console] Starting...");
  
  startTime = millis();
  
  // Initialize camera
  if (!initCamera()) {
    Serial.println("[tactical-console] Camera init failed!");
  } else {
    Serial.println("[tactical-console] Camera ready");
  }
  
  // Initialize MPU-6050 IMU
  if (!initIMU()) {
    Serial.println("[tactical-console] IMU init failed - continuing without IMU");
  } else {
    Serial.println("[tactical-console] IMU (MPU-6050) ready");
  }
  
  // Initialize WiFi
  if (AP_MODE) {
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("[tactical-console] AP Mode - Connect to: ");
    Serial.println(ap_ssid);
    Serial.print("[tactical-console] IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    Serial.print("[tactical-console] Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.print("[tactical-console] Connected - IP: ");
    Serial.println(WiFi.localIP());
  }
  
  // Start mDNS responder for http://tactical.local
  if (MDNS.begin(mdns_hostname)) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("[tactical-console] mDNS: http://");
    Serial.print(mdns_hostname);
    Serial.println(".local");
  } else {
    Serial.println("[tactical-console] mDNS failed to start");
  }
  
  // Setup HTTP routes
  setupRoutes();
  server.begin();
  
  Serial.println("[tactical-console] Server started");
}

void loop() {
  server.handleClient();
  if (imuAvailable) {
    readIMU();
  }
  delay(10);
}

// MPU-6050 IMU initialization
bool initIMU() {
  Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);
  Wire.setClock(400000);  // 400kHz I2C
  
  // Check if MPU-6050 is present
  Wire.beginTransmission(MPU6050_ADDR);
  if (Wire.endTransmission() != 0) {
    imuAvailable = false;
    return false;
  }
  
  // Wake up MPU-6050 (clear sleep bit)
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_PWR_MGMT_1);
  Wire.write(0x00);  // Clear sleep mode
  Wire.endTransmission(true);
  
  // Configure accelerometer: +/- 2g (default)
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x1C);  // ACCEL_CONFIG register
  Wire.write(0x00);  // +/- 2g
  Wire.endTransmission(true);
  
  // Configure gyroscope: +/- 250 deg/s (default)
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x1B);  // GYRO_CONFIG register
  Wire.write(0x00);  // +/- 250 deg/s
  Wire.endTransmission(true);
  
  imuAvailable = true;
  return true;
}

// Read IMU sensor data
void readIMU() {
  static unsigned long lastRead = 0;
  if (millis() - lastRead < 50) return;  // 20Hz update rate
  lastRead = millis();
  
  // Read 14 bytes starting from ACCEL_XOUT_H
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 14, true);
  
  if (Wire.available() < 14) return;
  
  // Accelerometer (raw values, 16384 LSB/g at +/-2g)
  int16_t rawAccelX = (Wire.read() << 8) | Wire.read();
  int16_t rawAccelY = (Wire.read() << 8) | Wire.read();
  int16_t rawAccelZ = (Wire.read() << 8) | Wire.read();
  
  // Temperature
  int16_t rawTemp = (Wire.read() << 8) | Wire.read();
  
  // Gyroscope (raw values, 131 LSB/(deg/s) at +/-250deg/s)
  int16_t rawGyroX = (Wire.read() << 8) | Wire.read();
  int16_t rawGyroY = (Wire.read() << 8) | Wire.read();
  int16_t rawGyroZ = (Wire.read() << 8) | Wire.read();
  
  // Convert to physical units
  accelX = rawAccelX / 16384.0;
  accelY = rawAccelY / 16384.0;
  accelZ = rawAccelZ / 16384.0;
  
  gyroX = rawGyroX / 131.0;
  gyroY = rawGyroY / 131.0;
  gyroZ = rawGyroZ / 131.0;
  
  temperature = (rawTemp / 340.0) + 36.53;
  
  // Calculate pitch and roll from accelerometer
  // (simple tilt sensing - works when relatively stationary)
  // Negated for correct orientation with MPU-6050 mounted component-side up
  pitch = -atan2(accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180.0 / PI;
  roll = -atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180.0 / PI;
}

// Non-blocking IMU read for use during streaming
void readIMUImmediate() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 14, true);
  
  if (Wire.available() < 14) return;
  
  int16_t rawAccelX = (Wire.read() << 8) | Wire.read();
  int16_t rawAccelY = (Wire.read() << 8) | Wire.read();
  int16_t rawAccelZ = (Wire.read() << 8) | Wire.read();
  int16_t rawTemp = (Wire.read() << 8) | Wire.read();
  int16_t rawGyroX = (Wire.read() << 8) | Wire.read();
  int16_t rawGyroY = (Wire.read() << 8) | Wire.read();
  int16_t rawGyroZ = (Wire.read() << 8) | Wire.read();
  
  accelX = rawAccelX / 16384.0;
  accelY = rawAccelY / 16384.0;
  accelZ = rawAccelZ / 16384.0;
  gyroX = rawGyroX / 131.0;
  gyroY = rawGyroY / 131.0;
  gyroZ = rawGyroZ / 131.0;
  temperature = (rawTemp / 340.0) + 36.53;
  
  pitch = -atan2(accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180.0 / PI;
  roll = -atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180.0 / PI;
}

// Camera initialization
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[tactical-console] Camera init error: 0x%x\n", err);
    return false;
  }
  
  sensor_t* s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  s->set_framesize(s, FRAMESIZE_QVGA);
  
  return true;
}

// HTTP route handlers
void setupRoutes() {
  // Static files - served from PROGMEM
  server.on("/", HTTP_GET, handleIndex);
  server.on("/index.html", HTTP_GET, handleIndex);
  server.on("/assets/main.js", HTTP_GET, handleMainJs);
  
  // API endpoints
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/telemetry", HTTP_GET, handleApiTelemetry);
  
  // Camera endpoints
  server.on("/api/capture", HTTP_GET, handleCapture);
  server.on("/api/stream", HTTP_GET, handleStream);
  
  // Toggle recording
  server.on("/api/record/toggle", HTTP_POST, handleRecordToggle);
  
  server.onNotFound(handleNotFound);
}

void handleIndex() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleMainJs() {
  server.send_P(200, "application/javascript", MAIN_JS);
}

void handleApiStatus() {
  unsigned long uptimeSeconds = (millis() - startTime) / 1000;
  
  String json = "{";
  json += "\"deviceId\":\"ESP32-S3-CAM\",";
  json += "\"uptimeSeconds\":" + String(uptimeSeconds) + ",";
  json += "\"version\":\"1.0.0\",";
  json += "\"recording\":" + String(isRecording ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleApiTelemetry() {
  String json = "{";
  json += "\"orientation\":{";
  json += "\"available\":" + String(imuAvailable ? "true" : "false") + ",";
  json += "\"pitch\":" + String(pitch, 2) + ",";
  json += "\"roll\":" + String(roll, 2) + ",";
  json += "\"accel\":{";
  json += "\"x\":" + String(accelX, 3) + ",";
  json += "\"y\":" + String(accelY, 3) + ",";
  json += "\"z\":" + String(accelZ, 3);
  json += "},";
  json += "\"gyro\":{";
  json += "\"x\":" + String(gyroX, 2) + ",";
  json += "\"y\":" + String(gyroY, 2) + ",";
  json += "\"z\":" + String(gyroZ, 2);
  json += "},";
  json += "\"temp\":" + String(temperature, 1);
  json += "},";
  json += "\"timestamp\":" + String(millis());
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleCapture() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }
  
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleStream() {
  WiFiClient client = server.client();
  
  isRecording = true;
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(response);
  
  unsigned long lastImuRead = 0;
  
  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("[tactical-console] Stream: capture failed");
      break;
    }
    
    String header = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + String(fb->len) + "\r\n\r\n";
    client.print(header);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    
    esp_camera_fb_return(fb);
    
    // Keep IMU data updated during streaming (every 50ms)
    if (imuAvailable && (millis() - lastImuRead > 50)) {
      readIMUImmediate();
      lastImuRead = millis();
    }
    
    if (!client.connected()) break;
    delay(33); // ~30fps max
  }
  
  isRecording = false;
}

void handleRecordToggle() {
  isRecording = !isRecording;
  String json = "{\"recording\":" + String(isRecording ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

