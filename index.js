// functions/index.js
import { initializeApp } from "firebase-admin/app";
import { logger } from "firebase-functions";
import { Storage } from "@google-cloud/storage";
import { ImageAnnotatorClient } from "@google-cloud/vision";
import { onRequest } from "firebase-functions/v2/https";

// ----------------------------------
// Food Database
// ----------------------------------
const foodDatabase = [
  { name: 'Apple', calories: 52, protein: 0.3, fat: 0.2, carbs: 13.8 },
  { name: 'Banana', calories: 89, protein: 1.1, fat: 0.3, carbs: 22.8 },
  { name: 'Orange', calories: 47, protein: 0.9, fat: 0.1, carbs: 11.8 },
  { name: 'Strawberries', calories: 33, protein: 0.7, fat: 0.3, carbs: 7.7 },
  { name: 'Grapes', calories: 69, protein: 0.7, fat: 0.2, carbs: 18.1 },
  { name: 'Watermelon', calories: 30, protein: 0.6, fat: 0.2, carbs: 7.6 },
  { name: 'Blueberries', calories: 57, protein: 0.7, fat: 0.3, carbs: 14.5 },
  { name: 'Pineapple', calories: 50, protein: 0.5, fat: 0.1, carbs: 13.1 },
  { name: 'Peach', calories: 39, protein: 0.9, fat: 0.3, carbs: 9.5 },

  { name: 'Potato (Baked)', calories: 93, protein: 2.5, fat: 0.1, carbs: 21.2 },
  { name: 'Sweet Potato (Baked)', calories: 90, protein: 2.0, fat: 0.2, carbs: 20.7 },
  { name: 'Carrot (Boiled)', calories: 35, protein: 0.8, fat: 0.2, carbs: 8.2 },
  { name: 'Broccoli (Steamed)', calories: 35, protein: 2.4, fat: 0.4, carbs: 7.2 },
  { name: 'Spinach (Boiled)', calories: 23, protein: 2.9, fat: 0.4, carbs: 3.7 },
  { name: 'Green Peas (Boiled)', calories: 84, protein: 5.4, fat: 0.4, carbs: 15.6 },
  { name: 'Corn (Boiled)', calories: 96, protein: 3.4, fat: 1.5, carbs: 21.6 },
  { name: 'Green Beans (Boiled)', calories: 35, protein: 1.8, fat: 0.2, carbs: 7.9 },
  { name: 'Tomato (Raw)', calories: 18, protein: 0.9, fat: 0.2, carbs: 3.9 },

  { name: 'White Rice (Boiled)', calories: 130, protein: 2.4, fat: 0.2, carbs: 28.7 },
  { name: 'Brown Rice (Boiled)', calories: 123, protein: 2.6, fat: 1.0, carbs: 25.6 },
  { name: 'Spaghetti (Boiled)', calories: 158, protein: 5.8, fat: 0.9, carbs: 30.9 },
  { name: 'White Bread', calories: 265, protein: 9.0, fat: 3.2, carbs: 49.4 },
  { name: 'Whole Wheat Bread', calories: 252, protein: 12.0, fat: 3.4, carbs: 43.0 },
  { name: 'Flour Tortilla', calories: 312, protein: 8.7, fat: 7.8, carbs: 50.8 },
  { name: 'Oatmeal (Cooked)', calories: 68, protein: 2.4, fat: 1.4, carbs: 12.0 },
  { name: 'Pancake', calories: 227, protein: 6.0, fat: 7.8, carbs: 33.0 },
  { name: 'Bagel (Plain)', calories: 250, protein: 10.0, fat: 1.5, carbs: 49.0 },

  { name: 'Chicken Breast (Roasted)', calories: 165, protein: 31.0, fat: 3.6, carbs: 0 },
  { name: 'Chicken Thigh (Roasted)', calories: 229, protein: 25.0, fat: 13.2, carbs: 0 },
  { name: 'Turkey (Roasted)', calories: 189, protein: 28.6, fat: 7.4, carbs: 0 },
  { name: 'Ground Beef (Pan-browned)', calories: 276, protein: 25.3, fat: 18.6, carbs: 0 },
  { name: 'Beef Steak (Grilled)', calories: 187, protein: 29.8, fat: 6.6, carbs: 0 },
  { name: 'Pork Chop (Broiled)', calories: 250, protein: 27.9, fat: 14.6, carbs: 0 },
  { name: 'Ham (Roasted)', calories: 246, protein: 18.5, fat: 18.5, carbs: 0.1 },
  { name: 'Bacon (Fried)', calories: 541, protein: 37.0, fat: 41.8, carbs: 1.4 },
  { name: 'Salmon (Baked)', calories: 206, protein: 22.1, fat: 12.0, carbs: 0 },
  { name: 'Shrimp (Boiled)', calories: 99, protein: 20.9, fat: 1.1, carbs: 0 },

  { name: 'Egg (Boiled)', calories: 147, protein: 12.6, fat: 9.9, carbs: 0.8 },
  { name: 'Milk (Whole)', calories: 61, protein: 3.2, fat: 3.3, carbs: 4.8 },
  { name: 'Cheddar Cheese', calories: 403, protein: 24.9, fat: 33.1, carbs: 1.3 },
  { name: 'Yogurt (Plain)', calories: 61, protein: 3.5, fat: 3.3, carbs: 4.7 }
];

// ----------------------------------
// Firebase & GCS Configuration
// ----------------------------------
initializeApp();
const BUCKET_NAME = "scale-project-7ed9a.firebasestorage.app";
const storage = new Storage();
const bucket = storage.bucket(BUCKET_NAME);
const visionClient = new ImageAnnotatorClient();

// ----------------------------------
// Helper Functions
// ----------------------------------

// Calculate Jaccard similarity between two sets.
function jaccardSimilarity(setA, setB) {
  const intersection = new Set([...setA].filter(x => setB.has(x)));
  const union = new Set([...setA, ...setB]);
  return intersection.size / union.size;
}

// Find the best matching food from the food database using the provided labels.
function findBestMatch(labels) {
  let bestFood = null;
  let highestScore = 0;
  
  for (const label of labels) {
    const labelWords = new Set(label.toLowerCase().split(/\s+/));
    for (const food of foodDatabase) {
      const foodWords = new Set(food.name.toLowerCase().split(/\s+/));
      const similarity = jaccardSimilarity(labelWords, foodWords);
      if (similarity > highestScore) {
        highestScore = similarity;
        bestFood = food;
      }
    }
  }
  return bestFood || { message: "No suitable match found." };
}

// ----------------------------------
// HTTP-triggered function to analyze the image manually
// ----------------------------------
export const analyzeImage = onRequest(async (req, res) => {
  try {
    // Get the file name from the query parameter; default to "data/photo.jpg"
    let fileName =
      typeof req.query.fileName === "string" ? req.query.fileName : "data/food.jpg";

    // Ensure the file is in the "data/" folder.
    if (!fileName.startsWith("data/")) {
      fileName = `data/${fileName}`;
    }
    logger.info("Processing file: " + fileName);

    // Basic path validation to prevent directory traversal.
    if (fileName.includes("..") || !fileName.match(/^[\w\-./]+$/)) {
      logger.error("Invalid file name attempted: " + fileName);
      res.status(400).json({ success: false, error: "Invalid file name." });
      return;
    }

    // Access the file in the bucket.
    const file = bucket.file(fileName);
    const [exists] = await file.exists();
    if (!exists) {
      logger.error("File not found: " + fileName);
      res.status(404).json({ success: false, error: "File not found." });
      return;
    }

    // Download the image.
    const [imageBuffer] = await file.download();
    if (!imageBuffer || !imageBuffer.length) {
      logger.error("Failed to download image: " + fileName);
      res.status(500).json({ success: false, error: "Failed to download image." });
      return;
    }
    logger.info(`Downloaded ${imageBuffer.length} bytes from ${fileName}`);

    // Prepare the Vision API request.
    const visionRequest = {
      image: { content: imageBuffer.toString("base64") },
      features: [{ type: "LABEL_DETECTION", maxResults: 10 }],
    };
    logger.info("Sending request to Vision API...");
    const [visionResult] = await visionClient.annotateImage(visionRequest);
    logger.info("Received response from Vision API.");

    // Process Vision API response.
    const labelAnnotations = visionResult.labelAnnotations;
    let visionLabels = [];
    if (labelAnnotations && labelAnnotations.length > 0) {
      visionLabels = labelAnnotations
        .map(label => label.description)
        .filter(desc => desc != null);
    }

    // Determine the best matching food.
    const matchedFood = findBestMatch(visionLabels);
    logger.info("Matched Food:", JSON.stringify(matchedFood, null, 2));

    // Format nutritional information as plain text.
    let resultText = "";
    if (matchedFood && matchedFood.name) {
      resultText = `Food: ${matchedFood.name}\n` +
                   `Calories: ${matchedFood.calories} kcal\n` +
                   `Protein: ${matchedFood.protein} g\n` +
                   `Fat: ${matchedFood.fat} g\n` +
                   `Carbs: ${matchedFood.carbs} g`;
    } else {
      resultText = "No suitable match found.";
    }

    // Overwrite the nutritional information in "results/results.txt".
    const resultFile = bucket.file("results/results.txt");
    await resultFile.save(resultText, { contentType: "text/plain" });
    logger.info("Results saved to results/results.txt");

    res.status(200).json({
      success: true,
      result: resultText,
      visionLabels,
    });
  } catch (error) {
    logger.error("Error processing image: " + error);
    res.status(500).json({ success: false, error: error.message || "Unexpected error." });
  }
});
