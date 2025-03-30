#include "Display_Module.h"

// Color definitions
#define COLOR_FAT     ST77XX_YELLOW
#define COLOR_CARBS   ST77XX_BLUE
#define COLOR_PROTEIN ST77XX_GREEN
#define COLOR_BG      ST77XX_BLACK
#define COLOR_TEXT    ST77XX_WHITE
#define COLOR_TITLE   ST77XX_CYAN

String truncateString(const String &str, int maxWidth, int textSize) {
  // Estimate the width per character (6 pixels per char at textSize 1)
  int charWidth = 6 * textSize;
  int baseMaxChars = maxWidth / charWidth;
  int maxChars = baseMaxChars + 5;
  if (str.length() > maxChars) {
    return str.substring(0, maxChars - 3) + "...";
  }
  return str;
}

DisplayModule::DisplayModule(uint8_t cs, uint8_t dc, uint8_t rst)
  : tft(cs, dc, rst) {
}

void DisplayModule::begin(uint16_t w, uint16_t h) {
  width = w;
  height = h;
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);
  tft.setTextWrap(true);
}

bool DisplayModule::downloadTextFile(const char* remotePath, const char* localPath) {
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

NutritionData DisplayModule::parseNutritionJSON(const String &inputText) {
  NutritionData data = {"Unknown", 0, 0, 0, 0};
  String trimmedInput = inputText;
  trimmedInput.trim();
  
  if (trimmedInput.startsWith("{")) {
    DynamicJsonDocument doc(1024);
    String cleanJson = trimmedInput;
    cleanJson.replace("```", "");
    DeserializationError error = deserializeJson(doc, cleanJson);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return data;
    }
    if (doc.containsKey("matchedFood")) {
      JsonObject food = doc["matchedFood"];
      if (food.containsKey("name")) {
        data.foodName = food["name"].as<String>();
      }
      if (food.containsKey("calories")) {
        data.calories = food["calories"].as<int>();
      }
      if (food.containsKey("fat")) {
        data.fat = food["fat"].as<float>();
      }
      if (food.containsKey("carbs")) {
        data.carbs = food["carbs"].as<float>();
      }
      if (food.containsKey("protein")) {
        data.protein = food["protein"].as<float>();
      }
    }
  } else {
    int lineStart = 0;
    while (lineStart < inputText.length()) {
      int lineEnd = inputText.indexOf('\n', lineStart);
      if (lineEnd == -1) {
        lineEnd = inputText.length();
      }
      String line = inputText.substring(lineStart, lineEnd);
      line.trim();
      if (line.length() > 0) {
        int colonIndex = line.indexOf(':');
        if (colonIndex != -1) {
          String key = line.substring(0, colonIndex);
          String value = line.substring(colonIndex + 1);
          key.trim();
          value.trim();
          if (key.equalsIgnoreCase("Food")) {
            data.foodName = value;
          } else if (key.equalsIgnoreCase("Calories")) {
            int spaceIndex = value.indexOf(' ');
            if (spaceIndex != -1) {
              value = value.substring(0, spaceIndex);
            }
            data.calories = value.toInt();
          } else if (key.equalsIgnoreCase("Protein")) {
            int spaceIndex = value.indexOf(' ');
            if (spaceIndex != -1) {
              value = value.substring(0, spaceIndex);
            }
            data.protein = value.toFloat();
          } else if (key.equalsIgnoreCase("Fat")) {
            int spaceIndex = value.indexOf(' ');
            if (spaceIndex != -1) {
              value = value.substring(0, spaceIndex);
            }
            data.fat = value.toFloat();
          } else if (key.equalsIgnoreCase("Carbs")) {
            int spaceIndex = value.indexOf(' ');
            if (spaceIndex != -1) {
              value = value.substring(0, spaceIndex);
            }
            data.carbs = value.toFloat();
          }
        }
      }
      lineStart = lineEnd + 1;
    }
  }
  
  Serial.printf("Parsed nutrition data: %s - Calories = %i, Fat = %.1f, Carbs = %.1f, Protein = %.1f\n",
                data.foodName.c_str(), data.calories, data.fat, data.carbs, data.protein);
  return data;
}

void DisplayModule::displayNutritionData(const NutritionData &data) {
  tft.fillScreen(COLOR_BG);
  int availableWidth = 120;
  String truncatedName = truncateString(data.foodName, availableWidth, 1);
  tft.setCursor(5, 2);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TITLE);
  tft.print(truncatedName);
  tft.setCursor(5, 15);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TITLE);
  tft.print("CALORIES");
  tft.setCursor(5, 35);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_TEXT);
  tft.print(int(data.calories));
  tft.setTextSize(1);
  tft.print(" kCal");
  
  float maxValue = max(max(data.fat, data.carbs), data.protein);
  maxValue = max(maxValue, 10.0f);
  
  tft.setCursor(5, 60);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TITLE);
  tft.print("MACRONUTRIENTS (g)");
  
  int totalBarWidth = 105;
  int barHeight = 12;
  int spacing = 20;
  int startY = 73;
  
  drawMacroBar(0, startY, totalBarWidth, barHeight, data.fat, maxValue, COLOR_FAT, "Fat");
  drawMacroBar(0, startY + spacing, totalBarWidth, barHeight, data.carbs, maxValue, COLOR_CARBS, "Carbs");
  drawMacroBar(0, startY + spacing * 2, totalBarWidth, barHeight, data.protein, maxValue, COLOR_PROTEIN, "Prot");
}

void DisplayModule::drawMacroBar(int x, int y, int totalBarWidth, int height, float value, float maxValue, uint16_t color, const char* label) {
  const int labelWidth = 40;
  tft.setCursor(5, y + height / 2 - 3);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.print(label);
  
  int barX = x + labelWidth;
  int barWidth = totalBarWidth - labelWidth;
  int barLength = (value * barWidth) / maxValue;
  tft.drawRect(barX, y, barWidth, height, COLOR_TEXT);
  tft.fillRect(barX, y, barLength, height, color);
  tft.setCursor(barX + barWidth + 2, y + height / 2 - 3);
  tft.setTextColor(COLOR_TEXT);
  tft.print(value, 1);
}

// NEW: Updated displayText() function that updates the global meal variable.
void DisplayModule::displayText(const String &text) {
  NutritionData data = parseNutritionJSON(text);
  displayNutritionData(data);
}
