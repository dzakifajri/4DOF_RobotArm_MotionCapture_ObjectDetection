#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// WiFi Configuration (Access Point)
const char* ssid = "ESP32_AccessPoint";
const char* password = "12345678"; // Minimal 8 karakter

// Network Configuration
const int PORT = 12345;
WiFiServer server(PORT);
WiFiClient client;

// Servo Pins
const int BASE_SERVO_PIN = 2;
const int SHOULDER_SERVO_PIN = 4;
const int ELBOW_SERVO_PIN = 15;
const int GRIPPER_SERVO_PIN = 13;

// Servo Objects
Servo baseServo;
Servo shoulderServo;
Servo elbowServo;
Servo gripperServo;

// Predefined Pick and Place Positions
struct PickPlacePositions {
  int baseAngle;
  int shoulderAngle;
  int elbowAngle;
  int gripperAngle;
};

// Predefined positions for each color
PickPlacePositions pickPosition = {90, 30, 45, 0};  // Pickup position
PickPlacePositions merahPlacePosition = {30, 45, 30, 60};  // Red place position
PickPlacePositions hijauPlacePosition = {120, 45, 30, 60};  // Green place position
PickPlacePositions biruPlacePosition = {180, 45, 30, 60};   // Blue place position

void setupWiFiAP() {
  // Start WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("WiFi started. IP address:");
  Serial.println(WiFi.softAPIP());
  
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupServos() {
  // Attach servos to pins
  baseServo.attach(BASE_SERVO_PIN, 500, 2500);     // 500-2500 µs pulse width
  shoulderServo.attach(SHOULDER_SERVO_PIN, 500, 2500);
  elbowServo.attach(ELBOW_SERVO_PIN, 500, 2500);
  gripperServo.attach(GRIPPER_SERVO_PIN, 500, 2500);

  // Initial position
  baseServo.write(90);  // Base servo at neutral position
  shoulderServo.write(0);
  elbowServo.write(0);
  gripperServo.write(0);
}

void smoothMoveServo(Servo& servo, int currentAngle, int targetAngle, int stepDelay) {
  if (currentAngle < targetAngle) {
    for (int angle = currentAngle; angle <= targetAngle; angle++) {
      servo.write(angle);
      delay(stepDelay);
    }
  } else {
    for (int angle = currentAngle; angle >= targetAngle; angle--) {
      servo.write(angle);
      delay(stepDelay);
    }
  }
}

void moveToPosition(PickPlacePositions pos) {
  static int currentBaseAngle = 90;      // Initial positions for all servos
  static int currentShoulderAngle = 0;
  static int currentElbowAngle = 0;
  static int currentGripperAngle = 0;

  // Move base servo smoothly
  smoothMoveServo(baseServo, currentBaseAngle, constrain(pos.baseAngle, 0, 180), 10);
  currentBaseAngle = constrain(pos.baseAngle, 0, 180);

  // Move shoulder servo smoothly
  smoothMoveServo(shoulderServo, currentShoulderAngle, constrain(pos.shoulderAngle, 0, 60), 10);
  currentShoulderAngle = constrain(pos.shoulderAngle, 0, 60);

  // Move elbow servo smoothly
  smoothMoveServo(elbowServo, currentElbowAngle, constrain(pos.elbowAngle, 0, 60), 10);
  currentElbowAngle = constrain(pos.elbowAngle, 0, 60);

  // Move gripper servo smoothly
  smoothMoveServo(gripperServo, currentGripperAngle, constrain(pos.gripperAngle, 0, 60), 10);
  currentGripperAngle = constrain(pos.gripperAngle, 0, 60);

  delay(500); // Allow time to stabilize
}

void pickAndPlace(const String& color) {
  Serial.println("Performing pick and place...");
  Serial.print("Detected color: ");
  Serial.println(color);
  
  // Move to pick position
  moveToPosition(pickPosition);
  delay(1000);
  
  // Close gripper to pick up object
  gripperServo.write(60);
  delay(500);
  
  // Move to appropriate place position based on color
  if (color == "Merah") {
    moveToPosition(merahPlacePosition);
  } else if (color == "Hijau") {
    moveToPosition(hijauPlacePosition);
  } else if (color == "Biru") {
    moveToPosition(biruPlacePosition);
  } else {
    // If unknown color, return to initial position
    moveToPosition({90, 0, 0, 0});
    Serial.println("Unknown color detected. Returning to initial position.");
    return;
  }
  
  // Open gripper to release object
  gripperServo.write(0);
  delay(500);
  
  // Return to initial position
  moveToPosition({90, 0, 0, 0});
}

void setup() {
  Serial.begin(115200);
  setupWiFiAP();  // Set ESP32 as Access Point
  setupServos();
}

void loop() {
  // Wait for client connection
  if (!client.connected()) {
    client = server.available();
    return;
  }
  
  // Check for incoming data
  if (client.available()) {
    String jsonString = client.readStringUntil('\n');
    Serial.println("Received JSON Data:");
    Serial.println(jsonString); // Print the raw JSON data
    
    // Parse JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      return;
    }
    
    // Process each tracked object
    for (JsonPair keyValue : doc.as<JsonObject>()) {
      String objId = keyValue.key().c_str();
      JsonObject objData = keyValue.value().as<JsonObject>();
      
      String color = objData["color"].as<String>();
      float x = objData["x"].as<float>();
      float y = objData["y"].as<float>();
      float z = objData["z"].as<float>();
      
      // Print object data
      Serial.print("Object ID: ");
      Serial.println(objId);
      Serial.print("Color: ");
      Serial.println(color);
      Serial.print("Coordinates - X: ");
      Serial.print(x);
      Serial.print(", Y: ");
      Serial.print(y);
      Serial.print(", Z: ");
      Serial.println(z);
      
      // Perform pick and place for this object
      pickAndPlace(color);
    }
  }
}
