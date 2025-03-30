#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include "soc/rtc.h"
#include "HX711.h"
#include <Firebase_ESP_Client.h>
#include <LittleFS.h>
#include <SPI.h>
#include "Display_Module.h"  // Include our new display module

// WiFi credentials
const char* ssid = "utexas-iot";
const char* password = "11453802769516735032";

// ESP32-CAM IP for triggering capture
const char* esp32CamIP = "10.159.66.102"; // Update as needed

// Positive logic button on D13 (GPIO13)
#define BUTTON_PIN 13

// Load cell inputs
#define LOADCELL_DOUT_PIN 18
#define LOADCELL_SCK_PIN 4

HX711 scale;

// Firebase credentials/configuration
#define API_KEY "AIzaSyC7TOTNIX31McbUnCL84GbzATQw9Vzx0cY"
#define USER_EMAIL "rajmhetar@gmail.com"
#define USER_PASSWORD "password"
#define STORAGE_BUCKET_ID_STR "scale-project-7ed9a.firebasestorage.app"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;
String STORAGE_BUCKET_ID = STORAGE_BUCKET_ID_STR;

// Create an instance of DisplayModule for the TFT display using your pin mapping:
// TFT CS: d5, D/C: d2, RESET: d15.
DisplayModule displayModule(5, 2, 15);

// Variables for periodic display refresh
unsigned long previousRefresh = 0;
const unsigned long refreshInterval = 15000; // Refresh every 15 seconds

void tokenStatusCallback(TokenInfo info) {
  Serial.printf("Token status: %d\n", info.status);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize button
  pinMode(BUTTON_PIN, INPUT);

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("Controller IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("\nPress the button on GPIO13 to trigger the ESP32-CAM.");

  // Set up load cell
  setCpuFrequencyMhz(80);
  rtc_cpu_freq_config_t config;
  rtc_clk_cpu_freq_get_config(&config);
  rtc_clk_cpu_freq_set_config_fast(&config);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    while (true) delay(1000);
  }
  Serial.println("LittleFS mounted successfully");

  // Initialize Firebase
  configF.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  configF.token_status_callback = tokenStatusCallback;  // Provided by Firebase_ESP_Client
  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize SPI with custom pin mapping:
  // SCK = d22, dummy MISO = d12 (unused), MOSI = d23, TFT CS = d5.
  SPI.begin(22, 12, 23, 5);

  // Turn on TFT backlight using d27
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);

  // Initialize TFT display via DisplayModule
  displayModule.begin();

  // Initially download and display the text file.
  const char* remoteTextFile = "results/results.txt";
  const char* localTextFile = "/textfile.txt";
  if (displayModule.downloadTextFile(remoteTextFile, localTextFile)) {
    String textContent = displayModule.readTextFile(localTextFile);
    Serial.println("Initial text content: " + textContent);
    displayModule.displayText(textContent);
  } else {
    Serial.println("Failed to download text file.");
  }
}

void loop() {
  // Button-triggered ESP32-CAM capture code
  if (digitalRead(BUTTON_PIN) == HIGH) {
    delay(50);  // Debounce delay
    if (digitalRead(BUTTON_PIN) == HIGH) {
      Serial.println("Button pressed! Sending /capture request to ESP32-CAM...");
      String url = "http://" + String(esp32CamIP) + "/capture";
      HTTPClient http;
      http.begin(url);
      int httpResponseCode = http.GET();
      if (httpResponseCode > 0) {
        Serial.print("ESP32-CAM Response Code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      http.end();
      // A small delay to prevent repeated triggers
      delay(1500);
    }
  }

  // Periodically refresh the display (every 15 seconds)
  unsigned long currentMillis = millis();
  if (currentMillis - previousRefresh >= refreshInterval) {
    previousRefresh = currentMillis;
    Serial.println("Refreshing display...");

    const char* remoteTextFile = "results/results.txt";
    const char* localTextFile = "/textfile.txt";
    if (displayModule.downloadTextFile(remoteTextFile, localTextFile)) {
      String jsonContent = displayModule.readTextFile(localTextFile);
      Serial.println("Refreshed JSON content: " + jsonContent);
      
      // Parse JSON nutrition data and display it visually
      NutritionData nutritionData = displayModule.parseNutritionJSON(jsonContent);
      displayModule.displayNutritionData(nutritionData);
    } else {
      Serial.println("Failed to refresh text file.");
    }

  }
}
