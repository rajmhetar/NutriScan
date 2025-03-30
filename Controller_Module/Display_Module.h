#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <LittleFS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <ArduinoJson.h>

// Extern declarations for global Firebase objects defined in your main sketch.
extern FirebaseData fbdo;
extern String STORAGE_BUCKET_ID;

// Structure to hold nutrition data
struct NutritionData {
  String foodName;
  int calories;
  float fat;
  float carbs;
  float protein;
};

// Declare the global current meal data variable.
extern NutritionData currentMealData;

class DisplayModule {
public:
  // Constructor: pass in TFT's CS, D/C, and RESET pins.
  DisplayModule(uint8_t cs, uint8_t dc, uint8_t rst);
  
  // Initialize the TFT display (default size 128x160)
  void begin(uint16_t width = 128, uint16_t height = 160);
  
  // Download a text file from Firebase Storage
  bool downloadTextFile(const char* remotePath, const char* localPath);
  
  // Read the downloaded file from LittleFS
  String readTextFile(const char* localPath);
  
  // Parse nutrition data from JSON or plain text format
  NutritionData parseNutritionJSON(const String &jsonText);
  
  // Display nutrition data with visual graph
  void displayNutritionData(const NutritionData &data);
  
  // Display text (parse, update global meal data, and show on LCD)
  void displayText(const String &text);
  
private:
  Adafruit_ST7735 tft;  // Instance of the TFT driver
  uint16_t width;
  uint16_t height;
  
  // Helper method for drawing macro nutrient bars
  void drawMacroBar(int x, int y, int width, int height, float value, float maxValue, uint16_t color, const char* label);
};

#endif
