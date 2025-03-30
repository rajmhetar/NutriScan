#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include "soc/rtc.h"
#include "HX711.h"
#include <Firebase_ESP_Client.h>
#include <LittleFS.h>
#include <SPI.h>
#include "Display_Module.h"  // Include our new display module

// Define the global currentMealData variable (definition)
NutritionData currentMealData = {"Unknown", 0, 0.0, 0.0, 0.0};

String currentUserId = "YH2zfPzlpqdwp5rHD62cGSxYtpL2";

void uploadCurrentMeal();

// WiFi credentials
const char* ssid = "WIFI";
const char* password = "PASSWORD";

// Positive logic button on D13 (GPIO13)
#define BUTTON_PIN 13

// Load cell inputs
#define LOADCELL_DOUT_PIN 18
#define LOADCELL_SCK_PIN 4

HX711 scale;

// Firebase credentials/configuration
#define API_KEY "APIKEY"
#define USER_EMAIL "EMAIL"
#define USER_PASSWORD "PASSWORD"
#define STORAGE_BUCKET_ID_STR "STORAGE_BUCKET_ID"
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
  if (digitalRead(BUTTON_PIN) == HIGH) {
    // Check that a meal has been displayed (using the global currentMealData)
    if (currentMealData.foodName != "" && currentMealData.foodName != "Unknown") {
      Serial.println("Button pressed: Uploading current meal for user " + currentUserId);
      uploadCurrentMeal();
      delay(1000); // Simple debounce delay; adjust as needed.
    } else {
      Serial.println("Button pressed but no valid meal data available.");
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
      
        displayModule.displayText(jsonContent);

    } else {
      Serial.println("Failed to refresh text file.");
    }
  }
}

void uploadCurrentMeal() {
  // For timestamp, we'll use a fixed placeholder for now.
  String ts = "2025-03-30T12:00:00Z";  // Replace with a proper RFC3339 timestamp if needed.
  
  // Build the payload string in Firestore format.
  String payload = "{";
  payload += "\"fields\": {";
  payload += "\"name\": {\"stringValue\": \"" + currentMealData.foodName + "\"},";
  payload += "\"calories\": {\"integerValue\": \"" + String(currentMealData.calories) + "\"},";
  payload += "\"timestamp\": {\"timestampValue\": \"" + ts + "\"},";
  payload += "\"userId\": {\"stringValue\": \"" + currentUserId + "\"},";
  payload += "\"nutrients\": {\"mapValue\": {\"fields\": {";
  payload += "\"fat\": {\"doubleValue\": \"" + String(currentMealData.fat, 2) + "\"},";
  payload += "\"carbs\": {\"doubleValue\": \"" + String(currentMealData.carbs, 2) + "\"},";
  payload += "\"protein\": {\"doubleValue\": \"" + String(currentMealData.protein, 2) + "\"}";
  payload += "}}}";
  payload += "}}";
  
  Serial.println("Payload: " + payload);
  
  // Create a document in Firestore under the "meals" collection.
  if (Firebase.Firestore.createDocument(&fbdo,
      "scale-project-7ed9a",  // Project ID
      "(default)",            // Database ID
      "meals",                // Collection ID
      "",                     // Document ID (auto-generate if empty)
      payload.c_str(),        // JSON content
      ""))                    // Mask (empty string)
  {
    Serial.println("Meal uploaded successfully.");
  } else {
    Serial.print("Meal upload failed: ");
    Serial.println(fbdo.errorReason());
  }
}
