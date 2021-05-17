// MASTER ARDUINO CODE
// Zander Rommelaere
// May 16, 2021

// This master board will control the ultrasonic sensors, LCD screen, servos,
// and IR sensors; it will determine the remaining logic to run the vehicle
// and send commands to the minion board to control the motors.

// External Libraries
#include <IRremote.h> // For IR receivers
#include <LiquidCrystal_I2C.h> // For LCD display
#include <NewPing.h> // For ultrasonic sensors
#include <Servo.h> // For servo motors
#include <SoftTimer.h> // For 'multi-threading': Running multiple tasks simultaniously
#include <Wire.h> // For I2C communication

// Define Task Methods
void mainTask(Task* me);
void LCDTask(Task* me);
void servoTask(Task* me);
void IRTask(Task* me);

// Define Tasks
Task task1(50, mainTask); // Run ultrasonics & transmit to minion every 50 ms
Task task2(100, LCDTask); // Update LCD every 100 ms
Task task3(70, servoTask); // Move servos every 70 ms
Task task4(100, IRTask); // Check IR sensors every 100 ms

// Define ultrasonic sensors
const int numOfSonars = 3;
const int maxSonarDistance = 100;

NewPing sonars[numOfSonars] = {
  NewPing(3, 3, maxSonarDistance),
  NewPing(2, 2, maxSonarDistance),
  NewPing(4, 4, maxSonarDistance)
};

// Define LCD & custom characters
LiquidCrystal_I2C LCD(0x27, 16, 2);

byte emptyChar[] = { // Blank character
  B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000
};

byte fullChar[] = { // Solid character
  B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111
};

// Define Servos
Servo servo1;
Servo servo2;

// Starting servo position & direction
int servoPos = 90;
bool servoIncreasing = true;

// Define Logic Variables
bool sonarLeftDetection = false;
bool sonarMidDetection = false;
bool sonarRightDetection = false;

byte controlMode = 0;

/* INTERNAL CONTROL MODE VALUES
 * 0 - IR Mode (control by IR remote with values below)
 * 1 - AI Mode (use ultrasonics to calculate path forward)
*/

/* IR REMOTE CONTROL VALUES - use remote buttons to control in IR mode
 * 0 - stop (also disable AI Mode if activated)
 * 1 - forward
 * 2 - backward
 * 3 - left
 * 4 - right
 * 
 * 6 - enable AI Mode (self driving)
*/

byte masterMotorValue = 0;

/* MOTOR CONTROL VALUES - Values sent to minion to control motors
 * 0 - stop
 * 1 - forward
 * 2 - backward
 * 3 - left
 * 4 - right
*/

void setup() {
  Serial.begin(9600); // Set to 9600 for debugging
  Wire.begin(); // Declare as master board

  LCD.init(); // Initialize LCD
  LCD.backlight(); // Enable LCD backlight
  LCD.createChar(0, emptyChar); // Create both custom characters
  LCD.createChar(1, fullChar);

  IrReceiver.begin(7); // Initialize IR receiver (running both sensors as one for technical reasons)

  servo1.attach(8); // Connect to servos
  servo2.attach(9);

  // Add tasks
  SoftTimer.add(&task1); // Main
  SoftTimer.add(&task2); // LCD
  SoftTimer.add(&task3); // Servos
  SoftTimer.add(&task4); // IR
}

// Determine proper sonar variable given reference num and set to the given value
void setSonar(byte sonarRefNum, bool newValue) {
  switch (sonarRefNum) {
    case 0:
      sonarLeftDetection = newValue;
      break;

    case 1:
      sonarMidDetection = newValue;
      break;

    case 2:
      sonarRightDetection = newValue;
      break;
  }
}

// Main Task including ultrasonics and AI Mode driving logic
void mainTask(Task* me) {
  if (controlMode != 1) { // Skip ultrasonics if not in AI Mode
    goto TRANSMIT;
  }

  // PING SONARS
  for (byte sonarRefNum = 0; sonarRefNum < numOfSonars; sonarRefNum++) { // For each ultrasonic sensor:
    long pingDistance = sonars[sonarRefNum].ping_cm(); // Ping distance
    if (pingDistance <= 35 && pingDistance != 0) { // If closer than 35cm
      setSonar(sonarRefNum, true); // Set variable as true for obstruction
    }
    else {
      setSonar(sonarRefNum, false); // Set variable as false for clear
    }
  }

  // LOGIC
  if (sonarMidDetection) { // If mid sensor obstruction:
    if (!sonarLeftDetection) { // If left sensor clear:
      masterMotorValue = 3; // Turn left
    }
    else if (!sonarRightDetection) { //Else, if right sensor clear:
      masterMotorValue = 4; // Turn right
    }
    else { // Otherwise: 
      masterMotorValue = 2; // Go backwards
    }
  }
  else if (sonarLeftDetection) { // If left sensor obstruction: turn right
    masterMotorValue = 4;
  }
  else if (sonarRightDetection) { // If right sensor obstruction: turn left
    masterMotorValue = 3;
  }
  else { // Otherwise: go forward
    masterMotorValue = 1;
  }

  TRANSMIT: // Transmit motor control values to minion board

  Wire.beginTransmission(8); // Start transmission
  Wire.write(masterMotorValue); // Queue motor control value
  Wire.endTransmission(); // Send motor control value
}

// Draw the 'detection icons' on the LCD
void WriteCustomLCD(int cursorPos, int charInt) {
  LCD.setCursor(cursorPos, 0);
  LCD.write(charInt);
  LCD.write(charInt);
  LCD.setCursor(cursorPos, 1);
  LCD.write(charInt);
  LCD.write(charInt);
}

// Update the LCD with information
void LCDTask(Task* me) {
  // Custom character 0 is empty (clear), 1 is full (detection)
  if (sonarLeftDetection) { // Left
    WriteCustomLCD(0, 1);
  }
  else {
    WriteCustomLCD(0, 0);
  }

  if (sonarMidDetection) { // Middle
    WriteCustomLCD(7, 1);
  }
  else {
    WriteCustomLCD(7, 0);
  }

  if (sonarRightDetection) { // Right
    WriteCustomLCD(14, 1);
  }
  else {
    WriteCustomLCD(14, 0);
  }

  // Show Control Mode
  LCD.setCursor(3, 0);
  LCD.print("MOD");

  LCD.setCursor(3, 1);
  if (controlMode == 0) { // IR MODE
    LCD.print("IRC");
  }
  else if (controlMode == 1) { // AI MODE
    LCD.print("AIC");
  }

  // Show Motor Control Value
  LCD.setCursor(10, 0);
  LCD.print("DIR");

  LCD.setCursor(11, 1);
//  LCD.print(masterMotorValue);
  switch (masterMotorValue) {
    case 0: // Stop
      LCD.print("-");
      break;

    case 1: // Forward
      LCD.print("F");
      break;

    case 2: // Backwards
      LCD.print("B");
      break;

    case 3: // Left
      LCD.print("L");
      break;

    case 4: // Right
      LCD.print("R");
      break;
  }
}

// Move the servos 2 degrees every 70 ms instead of one every 35 ms
void servoTask(Task* me) {
  if (controlMode == 1) { // Only move the servos in AI Mode
    if (servoPos == 74) { // Switch directions if at 74 or 104 degrees
      servoIncreasing = true;
    }
    else if (servoPos == 106) {
      servoIncreasing = false;
    }
  
    if (servoIncreasing) { // Increment or decrement
      servoPos += 2;
    }
    else {
      servoPos -= 2;
    }
  }
  else { // IR Mode; set to middle
    servoPos = 90;
  }

  servo1.write(servoPos); // Move servos to position
  servo2.write(servoPos);
}

// Check the IR Receiver for incoming signals
void IRTask(Task* me) {
  if (IrReceiver.decode()) { // If an IR signal has been detected
    switch (IrReceiver.decodedIRData.command) {
      case 22: // 0 on remote
        masterMotorValue = 0;
        controlMode = 0; // Set to IR Mode
        Serial.println("STOP");
        break;

      case 12: // 1 on remote
        if (controlMode == 0) { // If IR Mode
          masterMotorValue = 1; 
        }
        Serial.println("FORWARD");
        break;

      case 24: // 2 on remote
        if (controlMode == 0) {
          masterMotorValue = 2;
        }
        Serial.println("BACKWARD");
        break;

      case 94: // 3 on remote
        if (controlMode == 0) {
          masterMotorValue = 3;
        }
        Serial.println("LEFT");
        break;

      case 8: // 4 on remote
        if (controlMode == 0) {
          masterMotorValue = 4;
        }
        Serial.println("RIGHT");
        break;

      case 90: // 6 on remote
        masterMotorValue = 0;
        controlMode = 1; // Set to AI Mode
        Serial.println("AI MODE");
        break;
    }
    IrReceiver.resume(); // IMPORTANT: Continue listening for new signals
  }
}
