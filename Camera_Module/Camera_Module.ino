#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
#include <WebServer.h>
#include <addons/TokenHelper.h>

// WiFi Credentials
const char* ssid = "WIFI"; 
const char* password = "PASSWORD";

// Firebase Credentials
#define API_KEY "APIKEY"
#define USER_EMAIL "email"
#define USER_PASSWORD "password"
#define STORAGE_BUCKET_ID "STORAGE"
#define FILE_PHOTO_PATH "/photo.jpg"
#define BUCKET_PHOTO "/data/photo.jpg"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

WebServer server(80);

// Camera pin configuration for ESP32-CAM module (AI-Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// --- Firebase Storage Upload Callback ---
void fcsUploadCallback(FCS_UploadStatusInfo info) {
  if (info.status == firebase_fcs_upload_status_complete) {
    Serial.println("Upload completed");
    Serial.printf("Download URL: %s\n", fbdo.downloadURL().c_str());
  } else if (info.status == firebase_fcs_upload_status_error) {
    Serial.printf("Upload failed: %s\n", info.errorMsg.c_str());
  }
}

// --- Capture Photo and Save to LittleFS ---
void capturePhotoSaveLittleFS() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    esp_camera_fb_return(fb);
    return;
  }
  
  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  Serial.println("Photo saved to LittleFS");
}

// --- Upload Photo to Firebase ---
void uploadPhotoToFirebase() {
  if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO_PATH, mem_storage_type_flash, BUCKET_PHOTO, "image/jpeg", fcsUploadCallback)) {
    Serial.println("Firebase upload started");
  } else {
    Serial.printf("Firebase upload failed: %s\n", fbdo.errorReason().c_str());
  }
}

// --- Handle Web Requests ---
void handleCapture() {
  Serial.println("Capture request received. Taking photo...");
  capturePhotoSaveLittleFS();
  Serial.println("Uploading photo to Firebase...");
  uploadPhotoToFirebase();
  server.send(200, "text/plain", "Photo captured and uploaded to Firebase!");
}

// --- Initialize WiFi ---
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
}

// --- Initialize Camera ---
void initCamera() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
  Serial.println("Camera initialized");
}

// --- Initialize LittleFS ---
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    ESP.restart();
  }
  Serial.println("LittleFS mounted successfully");
}

// --- Initialize Firebase ---
void initFirebase() {
  configF.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  configF.token_status_callback = tokenStatusCallback;

  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  initWiFi();
  initLittleFS();
  initCamera();
  initFirebase();

Serial.print("ESP32-CAM IP Address: ");
Serial.println(WiFi.localIP());

  server.on("/capture", HTTP_GET, handleCapture);
  server.begin();

  Serial.println("Server started. Access via http://<ESP32-CAM-IP>/capture");
}

void loop() {
  server.handleClient();
}
