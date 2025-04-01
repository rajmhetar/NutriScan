// Firebase configuration (use your actual config values)
const firebaseConfig = {
  apiKey: "APIKEY",
  authDomain: "scale-project-7ed9a.firebaseapp.com",
  projectId: "scale-project-7ed9a",
  storageBucket: "scale-project-7ed9a.appspot.com",
  messagingSenderId: "484562709719",
  appId: "1:484562709719:web:c5895bd586fb1365c49282",
  measurementId: "G-R9SSWK6PKZ"  // Optional
};

// Initialize Firebase using the compat API
firebase.initializeApp(firebaseConfig);
const auth = firebase.auth();
const db = firebase.firestore();
// Enable long-polling (if needed)
db.settings({ experimentalAutoDetectLongPolling: true, merge: true });

console.log("Firebase initialized (namespaced SDK).");

// Global variable for editing
let editingMealId = null;

// Utility function to show/hide sections
function showSection(sectionId) {
  document.getElementById('auth-section').style.display = 'none';
  document.getElementById('dashboard-section').style.display = 'none';
  document.getElementById(sectionId).style.display = 'block';
}

// Authentication State Observer
auth.onAuthStateChanged((user) => {
  if (user) {
    showSection('dashboard-section');
    document.getElementById('welcome-message').innerText = `Welcome, ${user.email}!`;
    loadMeals();      // Load individual meal entries
    loadDailyMacros(); // Load total daily macronutrients
  } else {
    showSection('auth-section');
  }
});

// Handle Registration
document.getElementById('register-form').addEventListener('submit', (e) => {
  e.preventDefault();
  const emailElem = document.getElementById('register-email');
  const passwordElem = document.getElementById('register-password');
  const email = emailElem.value;
  const password = passwordElem.value;
  
  auth.createUserWithEmailAndPassword(email, password)
    .then((userCredential) => {
      console.log("NutriScan user registered:", userCredential.user.email);
      document.getElementById('register-output').innerText = "Registration successful! You can now log in.";
      emailElem.value = "";
      passwordElem.value = "";
    })
    .catch((error) => {
      console.error("Registration error:", error.message);
      document.getElementById('register-output').innerText = "Registration error: " + error.message;
    });
});

// Handle Login
document.getElementById('login-form').addEventListener('submit', (e) => {
  e.preventDefault();
  const email = document.getElementById('login-email').value;
  const password = document.getElementById('login-password').value;
  
  auth.signInWithEmailAndPassword(email, password)
    .then((userCredential) => {
      console.log("NutriScan user logged in:", userCredential.user.email);
      // Auth state observer will automatically switch the UI to dashboard
    })
    .catch((error) => {
      console.error("Login error:", error.message);
      document.getElementById('login-output').innerText = "Login error: " + error.message;
    });
});

// Handle Google Sign-In
document.getElementById('google-signin-btn').addEventListener('click', () => {
  const provider = new firebase.auth.GoogleAuthProvider();
  auth.signInWithPopup(provider)
    .then((result) => {
      console.log("Google sign in successful:", result.user.email);
    })
    .catch((error) => {
      console.error("Google sign in error:", error.message);
      document.getElementById('login-output').innerText = "Google sign in error: " + error.message;
    });
});

// Handle Logout
document.getElementById('logout-btn').addEventListener('click', () => {
  auth.signOut().then(() => {
    console.log("User signed out.");
  }).catch((error) => {
    console.error("Sign out error:", error.message);
  });
});

// Handle Meal Logging / Editing (only in dashboard)
const mealLogForm = document.getElementById('meal-log-form');
if (mealLogForm) {
  mealLogForm.addEventListener('submit', (e) => {
    e.preventDefault();
    console.log("Current user:", auth.currentUser);
    
    // Get meal data from the form
    const mealName = document.getElementById('meal-name').value;
    const mealCalories = parseInt(document.getElementById('meal-calories').value, 10);
    // Expect nutrient input as "fat, carbs, protein"
    const nutrientValues = document.getElementById('meal-nutrients').value.split(',').map(s => s.trim());
    const mealNutrients = {
      fat: parseFloat(nutrientValues[0]) || 0,
      carbs: parseFloat(nutrientValues[1]) || 0,
      protein: parseFloat(nutrientValues[2]) || 0
    };
  
    // Create a meal object to store
    const mealData = {
      name: mealName,
      calories: mealCalories,
      nutrients: mealNutrients,
      timestamp: firebase.firestore.FieldValue.serverTimestamp(),
      userId: auth.currentUser ? auth.currentUser.uid : null
    };
    console.log("Meal data:", mealData);
    
    if (editingMealId) {
      // Update existing meal document
      db.collection("meals").doc(editingMealId).update(mealData)
        .then(() => {
          console.log("Meal updated with ID:", editingMealId);
          document.getElementById('meal-log-output').innerText = "Meal updated successfully!";
          editingMealId = null;
          document.getElementById('meal-form-title').innerText = "Log Your Meal";
          document.getElementById('submit-meal-btn').innerText = "Submit Meal";
          document.getElementById('cancel-edit-btn').style.display = "none";
          e.target.reset();
          loadMeals();
          loadDailyMacros();
        })
        .catch((error) => {
          console.error("Error updating meal: ", error);
          document.getElementById('meal-log-output').innerText = "Failed to update meal.";
        });
    } else {
      // Add new meal document
      db.collection("meals").add(mealData)
        .then((docRef) => {
          console.log("Meal logged with ID:", docRef.id);
          document.getElementById('meal-log-output').innerText = "Meal logged successfully!";
          e.target.reset();
          loadMeals();
          loadDailyMacros();
        })
        .catch((error) => {
          console.error("Error adding meal: ", error);
          document.getElementById('meal-log-output').innerText = "Failed to log meal.";
        });
    }
  });
}

// Function to load and display the current user's meal logs with edit/delete buttons and statistics
function loadMeals() {
  if (!auth.currentUser) return;
  
  // Build query, optionally applying date filters if set
  let query = db.collection("meals")
                .where("userId", "==", auth.currentUser.uid)
                .orderBy("timestamp", "desc");
  
  // Apply date filtering if start/end dates are provided
  const startDate = document.getElementById("start-date").value;
  const endDate = document.getElementById("end-date").value;
  if (startDate) {
    const startTimestamp = new Date(startDate);
    query = query.where("timestamp", ">=", startTimestamp);
  }
  if (endDate) {
    const endTimestamp = new Date(endDate);
    endTimestamp.setHours(23,59,59,999);
    query = query.where("timestamp", "<=", endTimestamp);
  }
  
  query.get()
    .then((querySnapshot) => {
      let outputHtml = "<h3>Your Meal Log</h3>";
      let totalCalories = 0;
      querySnapshot.forEach((doc) => {
        let meal = doc.data();
        totalCalories += meal.calories || 0;
        let nutrientDisplay = "";
        if (typeof meal.nutrients === "object") {
          nutrientDisplay = `Fat: ${meal.nutrients.fat}, Carbs: ${meal.nutrients.carbs}, Protein: ${meal.nutrients.protein}`;
        } else if (Array.isArray(meal.nutrients)) {
          nutrientDisplay = meal.nutrients.join(", ");
        }
        outputHtml += `<div class="meal-entry">
                         <strong>${meal.name}</strong> - ${meal.calories} kCal<br>
                         Nutrients: ${nutrientDisplay}<br>
                         <button class="btn edit-btn" onclick="editMeal('${doc.id}', '${meal.name}', ${meal.calories}, '${meal.nutrients.fat},${meal.nutrients.carbs},${meal.nutrients.protein}')">Edit</button>
                         <button class="btn delete-btn" onclick="deleteMeal('${doc.id}')">Delete</button>
                       </div>`;
      });
      document.getElementById("meal-log-output").innerHTML = outputHtml;
      // Update statistics section (we'll update daily macros separately)
      document.getElementById("statistics-output").innerHTML = `<p>Total Calories Today: ${totalCalories} kCal</p>`;
    })
    .catch((error) => {
      console.error("Error loading meals:", error);
      document.getElementById("meal-log-output").innerText = "Failed to load meals.";
    });
}

// Function to delete a meal document
function deleteMeal(docId) {
  if (!confirm("Are you sure you want to delete this meal?")) return;
  
  db.collection("meals").doc(docId).delete()
    .then(() => {
      console.log("Meal deleted with ID:", docId);
      loadMeals();
      loadDailyMacros();
    })
    .catch((error) => {
      console.error("Error deleting meal:", error);
      alert("Failed to delete meal.");
    });
}

// Function to edit a meal: pre-fills the meal log form with current data
function editMeal(docId, name, calories, nutrientStr) {
  editingMealId = docId;
  document.getElementById('meal-name').value = name;
  document.getElementById('meal-calories').value = calories;
  document.getElementById('meal-nutrients').value = nutrientStr;
  document.getElementById('meal-form-title').innerText = "Edit Meal";
  document.getElementById('submit-meal-btn').innerText = "Update Meal";
  document.getElementById('cancel-edit-btn').style.display = "block";
}

// Handle Cancel Edit
document.getElementById('cancel-edit-btn').addEventListener('click', () => {
  editingMealId = null;
  document.getElementById('meal-log-form').reset();
  document.getElementById('meal-form-title').innerText = "Log Your Meal";
  document.getElementById('submit-meal-btn').innerText = "Submit Meal";
  document.getElementById('cancel-edit-btn').style.display = "none";
});

// Filter button event listener
document.getElementById('filter-btn').addEventListener('click', () => {
  loadMeals(); // Reload meals with current filter inputs
});

// NEW FUNCTION: Load and display total macronutrients for the current day
function loadDailyMacros() {
  if (!auth.currentUser) return;
  
  // Get current day boundaries
  const now = new Date();
  const startOfDay = new Date(now.getFullYear(), now.getMonth(), now.getDate());
  const endOfDay = new Date(now.getFullYear(), now.getMonth(), now.getDate(), 23, 59, 59, 999);
  
  db.collection("meals")
    .where("userId", "==", auth.currentUser.uid)
    .where("timestamp", ">=", startOfDay)
    .where("timestamp", "<=", endOfDay)
    .get()
    .then((querySnapshot) => {
      let totalFat = 0;
      let totalCarbs = 0;
      let totalProtein = 0;
      let totalCalories = 0;
      
      querySnapshot.forEach((doc) => {
        const meal = doc.data();
        totalCalories += meal.calories || 0;
        if (meal.nutrients) {
          totalFat += parseFloat(meal.nutrients.fat) || 0;
          totalCarbs += parseFloat(meal.nutrients.carbs) || 0;
          totalProtein += parseFloat(meal.nutrients.protein) || 0;
        }
      });
      
      const statsHtml = `
        <p>Total Calories Today: ${totalCalories} kCal</p>
        <p>Total Fat: ${totalFat.toFixed(1)} g, Total Carbs: ${totalCarbs.toFixed(1)} g, Total Protein: ${totalProtein.toFixed(1)} g</p>
      `;
      document.getElementById("statistics-output").innerHTML = statsHtml;
    })
    .catch((error) => {
      console.error("Error loading daily macros:", error);
      document.getElementById("statistics-output").innerText = "Failed to load daily macros.";
    });
}
