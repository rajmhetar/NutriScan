#include <WiFi.h>
#include <HTTPClient.h>

// Use the same WiFi credentials
const char* ssid = "utexas-iot";
const char* password = "11453802769516735032";


const char* esp32CamIP = "10.159.66.102"; // Update as needed

// Positive logic button on D13 (GPIO13)
#define BUTTON_PIN 13

void setup() {
  Serial.begin(115200);

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
}

void loop() {
  // If the button is pressed (goes HIGH)
  if (digitalRead(BUTTON_PIN) == HIGH) {
    // Debounce delay to avoid multiple triggers
    delay(50);
    if (digitalRead(BUTTON_PIN) == HIGH) {
      Serial.println("Button pressed! Sending /capture request to ESP32-CAM...");

      // Construct the full URL for the ESP32-CAM
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
}
