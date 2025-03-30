## Inspiration 
We wanted to create a seamless way to track food intake and nutritional information using embedded systems and cloud technology. Many existing food tracking apps require manual input, which can be time-consuming and inaccurate. With NutriScan, we automate this process by using an embedded camera and a cloud vision API to identify food and log it instantly.

## What it does 
NutriScan captures an image of food, identifies it using a cloud vision API, and displays the name along with detailed nutritional information on an LCD screen. This data is also sent to our website, where users can track their calorie intake and manage personal dietary goals in real time.

## How we built it 
- **Embedded System:** Used C/C++ on an ESP 32 to interface with the camera and LCD.
- **Cloud Vision API:** Wrote JavaScript functions to process images and return food identification results.
- **IoT Integration:** Connected the ESP 32 to the internet for wireless communication between the camera, LCD, and the cloud.
- **Website:** Built with HTML and CSS, where users create profiles, set goals, and track their food intake.
- **Real-Time Logging:** When a user logs a food item, it is instantly added to their profile, and the calories are automatically counted.

## Challenges we ran into 
- **Hardware Integration:** Ensuring smooth communication between the camera, LCD, and cloud API required debugging hardware-level issues.
- **IoT Connectivity:** Establishing a reliable wireless link between the embedded system and the website posed challenges with latency and data synchronization.
- **Frontend & Backend Sync:** Keeping the website updated in real-time with logged food entries required optimizing data flow.

## Accomplishments that we're proud of 
- Successfully integrating embedded hardware with cloud-based AI.
- Building a real-time food tracking system that is both efficient and user-friendly.
- Overcoming IoT communication challenges to ensure seamless data transfer.
- Developing a functional and visually appealing web interface for users to manage their dietary goals.

## What we learned 
- How to interface embedded systems with cloud APIs for real-world applications.
- Optimizing IoT connectivity for stable and efficient data transmission.
- Enhancing full-stack development skills by integrating hardware with a web-based platform.

## What's next for NutriScan 
- **Expanding Food Database:** Improve food recognition accuracy by integrating more datasets and AI training.
- **Mobile App Integration:** Develop a mobile version to provide users with a more accessible food tracking experience.
- **Advanced Health Insights:** Incorporate more nutritional data, such as macronutrient breakdowns and meal recommendations.
- **Wearable Compatibility:** Explore linking NutriScan data with smartwatches and fitness trackers for a holistic health-tracking experience.
