#include <ESP32Servo.h>

// Define servo pins
#define SERVO1_PIN 2  // Base rotation
#define SERVO2_PIN 4  // Shoulder
#define SERVO3_PIN 15  // Elbow
#define SERVO4_PIN 13  // Gripper

// Define potentiometer pins
#define POT1_PIN 32  
#define POT2_PIN 35 
#define POT3_PIN 34 
#define POT4_PIN 33  

// Define pushbutton pins
#define BUTTON_RECORD 5    // Button untuk record
#define BUTTON_STOP 18      // Button untuk stop
#define BUTTON_PLAY 19      // Button untuk play
#define BUTTON_RETURN 21    // Button untuk return/stop playing

// Create servo objects
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// Variables to store servo positions
int servo1_pos = 90;
int servo2_pos = 90;
int servo3_pos = 90;
int servo4_pos = 90;

// Variables to store initial positions
int initial_pos1 = 90;
int initial_pos2 = 90;
int initial_pos3 = 90;
int initial_pos4 = 90;

// Button states and timers
bool buttonStates[4] = {HIGH, HIGH, HIGH, HIGH};
bool lastButtonStates[4] = {HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[4] = {0, 0, 0, 0};
bool buttonPressed[4] = {false, false, false, false};

// Constants for recording
const int MAX_MOTION_STEPS = 500;  // Maximum number of steps that can be recorded

// Arrays to store recorded positions
int recordedPositions1[MAX_MOTION_STEPS];
int recordedPositions2[MAX_MOTION_STEPS];
int recordedPositions3[MAX_MOTION_STEPS];
int recordedPositions4[MAX_MOTION_STEPS];

// Recording variables
bool isRecording = false;
bool isPlaying = false;
int currentStep = 0;
int playIndex = 0;

unsigned long lastRecordTime = 0;
unsigned long lastPlayTime = 0;
const unsigned long RECORD_INTERVAL = 50;  // Record every 50ms
const unsigned long PLAY_INTERVAL = 50;    // Play every 50ms
const unsigned long DEBOUNCE_DELAY = 50;   // Debounce time in milliseconds

void setup() {
  Serial.begin(115200);

  // Setup button pins
  pinMode(BUTTON_RECORD, INPUT_PULLUP);
  pinMode(BUTTON_STOP, INPUT_PULLUP);
  pinMode(BUTTON_PLAY, INPUT_PULLUP);
  pinMode(BUTTON_RETURN, INPUT_PULLUP);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, 500, 2400);
  servo3.setPeriodHertz(50);
  servo3.attach(SERVO3_PIN, 500, 2400);
  servo4.setPeriodHertz(50);
  servo4.attach(SERVO4_PIN, 500, 2400);

  moveAllServos();
}

bool checkButton(int buttonPin, int buttonIndex) {
  bool buttonValue = digitalRead(buttonPin);
  
  if (buttonValue != lastButtonStates[buttonIndex]) {
    lastDebounceTime[buttonIndex] = millis();
  }
  
  if ((millis() - lastDebounceTime[buttonIndex]) > DEBOUNCE_DELAY) {
    if (buttonValue != buttonStates[buttonIndex]) {
      buttonStates[buttonIndex] = buttonValue;
      if (buttonStates[buttonIndex] == LOW && !buttonPressed[buttonIndex]) {
        buttonPressed[buttonIndex] = true;
        lastButtonStates[buttonIndex] = buttonValue;
        return true;
      } else if (buttonStates[buttonIndex] == HIGH) {
        buttonPressed[buttonIndex] = false;
      }
    }
  }
  
  lastButtonStates[buttonIndex] = buttonValue;
  return false;
}

void moveAllServos() {
  servo1.write(servo1_pos);
  servo2.write(servo2_pos);
  servo3.write(servo3_pos);
  servo4.write(servo4_pos);
}

void recordPosition() {
  if (currentStep < MAX_MOTION_STEPS && millis() - lastRecordTime >= RECORD_INTERVAL) {
    recordedPositions1[currentStep] = servo1_pos;
    recordedPositions2[currentStep] = servo2_pos;
    recordedPositions3[currentStep] = servo3_pos;
    recordedPositions4[currentStep] = servo4_pos;
    currentStep++;
    lastRecordTime = millis();
    
    Serial.print("Recording step: ");
    Serial.println(currentStep);
  }
}

void playRecordedMotion() {
  if (millis() - lastPlayTime >= PLAY_INTERVAL) {
    if (playIndex < currentStep) {
      servo1_pos = recordedPositions1[playIndex];
      servo2_pos = recordedPositions2[playIndex];
      servo3_pos = recordedPositions3[playIndex];
      servo4_pos = recordedPositions4[playIndex];
      
      moveAllServos();
      playIndex++;
      lastPlayTime = millis();
      
      Serial.print("Playing step: ");
      Serial.println(playIndex);
    } else {
      isPlaying = false;
      Serial.println("Playback finished");
    }
  }
}

void readPotentiometers() {
  servo1_pos = map(analogRead(POT1_PIN), 0, 4095, 0, 180);
  servo2_pos = map(analogRead(POT2_PIN), 0, 4095, 0, 60);
  servo3_pos = map(analogRead(POT3_PIN), 0, 4095, 0, 60);
  servo4_pos = map(analogRead(POT4_PIN), 0, 4095, 0, 180);
}

void returnToInitialPosition() {
  servo1_pos = initial_pos1;
  servo2_pos = initial_pos2;
  servo3_pos = initial_pos3;
  servo4_pos = initial_pos4;
  moveAllServos();
  Serial.println("Returned to initial position");
}

void handleButtons() {
  // Record button
  if (checkButton(BUTTON_RECORD, 0)) {
    if (!isRecording && !isPlaying) {
      isRecording = true;
      currentStep = 0;
      // Store initial positions
      initial_pos1 = servo1_pos;
      initial_pos2 = servo2_pos;
      initial_pos3 = servo3_pos;
      initial_pos4 = servo4_pos;
      Serial.println("Recording started");
    }
  }
  
  // Stop button
  if (checkButton(BUTTON_STOP, 1)) {
    if (isRecording) {
      isRecording = false;
      returnToInitialPosition();
      Serial.println("Recording stopped");
    }
  }
  
  // Play button
  if (checkButton(BUTTON_PLAY, 2)) {
    if (!isRecording && !isPlaying && currentStep > 0) {
      isPlaying = true;
      playIndex = 0;
      Serial.println("Playing recorded motion");
    }
  }
  
  // // Return/Stop playing button
  // if (checkButton(BUTTON_RETURN, 3)) {
  //   if (isPlaying) {
  //     isPlaying = false;
  //     returnToInitialPosition();
  //     Serial.println("Playing stopped");
  //   }
  // }
}

void loop() {
  handleButtons();

  if (isRecording) {
    readPotentiometers();
    moveAllServos();
    recordPosition();
  } else if (isPlaying) {
    playRecordedMotion();
  } else {
    readPotentiometers();
    moveAllServos();
  }

  delay(20);
}