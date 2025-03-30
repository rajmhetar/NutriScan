#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <LittleFS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Extern declarations for global Firebase objects defined in your main sketch.
extern FirebaseData fbdo;
extern String STORAGE_BUCKET_ID;

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

  // Display the text on the TFT
  void displayText(const String &text);

private:
  Adafruit_ST7735 tft;  // Instance of the TFT driver
};

#endif
