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
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "web_assets.h"  // Generated from frontend build

#define CAMERA_MODEL_XIAO_ESP32S3
#include "camera_pins.h"

// WiFi credentials - change these for your network
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Set to true for AP mode (device creates its own network)
const bool AP_MODE = true;
const char* ap_ssid = "TacticalConsole";
const char* ap_password = "tactical123";

WebServer server(80);

// Device state
unsigned long startTime;
bool isRecording = false;
float mockLat = 33.4484;
float mockLon = -112.0740;
float mockSpeed = 0;
float mockHeading = 0;

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
  
  // Setup HTTP routes
  setupRoutes();
  server.begin();
  
  Serial.println("[tactical-console] Server started");
}

void loop() {
  server.handleClient();
  updateMockTelemetry();
  delay(10);
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
  json += "\"lat\":" + String(mockLat, 6) + ",";
  json += "\"lon\":" + String(mockLon, 6) + ",";
  json += "\"speedKph\":" + String((int)mockSpeed) + ",";
  json += "\"headingDeg\":" + String((int)mockHeading);
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
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(response);
  
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
    
    if (!client.connected()) break;
    delay(33); // ~30fps max
  }
}

void handleRecordToggle() {
  isRecording = !isRecording;
  String json = "{\"recording\":" + String(isRecording ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

// Mock telemetry - replace with real GPS/IMU reads
void updateMockTelemetry() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 100) return;
  lastUpdate = millis();
  
  float t = millis() / 1000.0;
  mockLat = 33.4484 + sin(t / 10.0) * 0.001;
  mockLon = -112.0740 + cos(t / 10.0) * 0.001;
  mockSpeed = abs(sin(t / 5.0) * 80) + 20;
  mockHeading = fmod(t * 2, 360);
}

