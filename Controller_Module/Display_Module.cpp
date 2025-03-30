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
  // Compute the base maximum number of characters that can fit
  int baseMaxChars = maxWidth / charWidth;
  // Add an extra allowance of 5 characters before truncating
  int maxChars = baseMaxChars + 5;
  
  if (str.length() > maxChars) {
    // Reserve 3 characters for "..."
    return str.substring(0, maxChars - 3) + "...";
  }
  return str;
}

DisplayModule::DisplayModule(uint8_t cs, uint8_t dc, uint8_t rst)
  : tft(cs, dc, rst) {
}

void DisplayModule::begin(uint16_t w, uint16_t h) {
  // Initialize the TFT; INITR_BLACKTAB is typical for ST7735R displays.
  width = w;
  height = h;
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);         // Landscape mode for more graph space (128x160 becomes 160x128)
  tft.fillScreen(COLOR_BG);
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

NutritionData DisplayModule::parseNutritionJSON(const String &jsonText) {
  NutritionData data = {"Unknown", 0, 0, 0, 0};
  
  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument doc(1024);
  
  // Clean the jsonText by removing backticks and extra newlines
  String cleanJson = jsonText;
  cleanJson.replace("```", "");
  
  // Parse JSON object
  DeserializationError error = deserializeJson(doc, cleanJson);
  
  // Test if parsing succeeds
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return data;
  }
  
  // Extract values
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
  
  Serial.printf("Parsed nutrition data: %s - Calories = %i, Fat = %.1f, Carbs = %.1f, Protein = %.1f\n", 
                data.foodName.c_str(), data.calories, data.fat, data.carbs, data.protein);
                
  return data;
}

void DisplayModule::displayNutritionData(const NutritionData &data) {
  tft.fillScreen(COLOR_BG);
  
  // Define available width for the food name (adjust as needed for your layout)
  int availableWidth = 120;  // Example: 120 pixels available for the food name
  
  // Truncate the food name if it's too long using text size 1
  String truncatedName = truncateString(data.foodName, availableWidth, 1);
  
  // Display truncated food name
  tft.setCursor(5, 2);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TITLE);
  tft.print(truncatedName);
  
  // Display calories as large number
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
  
  // Calculate maximum value for scaling the bars (minimum 10g for visualization)
  float maxValue = max(max(data.fat, data.carbs), data.protein);
  maxValue = max(maxValue, 10.0f);
  
  // Draw title for macronutrients section
  tft.setCursor(5, 60);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TITLE);
  tft.print("MACRONUTRIENTS (g)");
  
  // Define dimensions for bars
  int totalBarWidth = 105; // Total width for label + bar
  int barHeight = 12;      // Bar height
  int spacing = 20;        // Vertical spacing between bars
  int startY = 73;         // Starting y-coordinate for the first bar
  
  // Draw macro bars (use abbreviated labels as needed)
  drawMacroBar(0, startY, totalBarWidth, barHeight, data.fat, maxValue, COLOR_FAT, "Fat");
  drawMacroBar(0, startY + spacing, totalBarWidth, barHeight, data.carbs, maxValue, COLOR_CARBS, "Carbs");
  drawMacroBar(0, startY + spacing*2, totalBarWidth, barHeight, data.protein, maxValue, COLOR_PROTEIN, "Prot");
}


void DisplayModule::drawMacroBar(int x, int y, int totalBarWidth, int height, float value, float maxValue, uint16_t color, const char* label) {
  // Reserve a fixed width for the label (e.g., 40 pixels)
  const int labelWidth = 40;
  
  // Draw the label in the reserved label area (left side)
  tft.setCursor(5, y + height/2 - 3);  // Adjust as needed for centering
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.print(label);
  
  // Define the starting x-coordinate for the bar as after the label area
  int barX = x + labelWidth;
  int barWidth = totalBarWidth - labelWidth;  // The remaining width is for the bar
  
  // Calculate bar length proportional to value
  int barLength = (value * barWidth) / maxValue;
  
  // Draw bar outline
  tft.drawRect(barX, y, barWidth, height, COLOR_TEXT);
  
  // Fill bar according to value
  tft.fillRect(barX, y, barLength, height, color);
  
  // Optionally, draw the numeric value at the right edge of the bar
  tft.setCursor(barX + barWidth + 2, y + height/2 - 3);
  tft.setTextColor(COLOR_TEXT);
  tft.print(value, 1);
}


void DisplayModule::displayText(const String &text) {
  // Parse and display nutrition data
  NutritionData data = parseNutritionJSON(text);
  displayNutritionData(data);
}