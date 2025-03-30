#include "Display_Module.h"

DisplayModule::DisplayModule(uint8_t cs, uint8_t dc, uint8_t rst)
  : tft(cs, dc, rst) {
}

void DisplayModule::begin(uint16_t width, uint16_t height) {
  // Initialize the TFT; INITR_BLACKTAB is typical for ST7735R displays.
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);         // Adjust rotation as needed.
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(true);
}

bool DisplayModule::downloadTextFile(const char* remotePath, const char* localPath) {
  // Download text file from Firebase Storage with content type "text/plain".
  if (Firebase.Storage.download(&fbdo, STORAGE_BUCKET_ID, remotePath, localPath, mem_storage_type_flash)) {
    Serial.println("Text file downloaded successfully.");
    return true;
  } else {
    Serial.printf("Download failed: %s\n", fbdo.errorReason().c_str());
    return false;
  }
}

String DisplayModule::readTextFile(const char* localPath) {
  File file = LittleFS.open(localPath, "r");
  if (!file) {
    Serial.println("Failed to open file from LittleFS");
    return "";
  }
  String content = file.readString();
  file.close();
  return content;
}

void DisplayModule::displayText(const String &text) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);  // Adjust text size as needed.
  tft.print(text);
}
