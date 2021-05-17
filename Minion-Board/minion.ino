// MINION ARDUINO CODE
// Zander Rommelaere
// May 16, 2021

// This minion board will control the motors based
// on the commands it receives from the master board.

// External Libraries
#include <SoftTimer.h> // For 'multi-threading': Running multiple tasks simultaniously
#include <Wire.h> // For I2C communication

// Define Task Methods
void motorTask(Task* me);

// Tasks
Task task1(50, motorTask); // Manage motors every 50 ms

// Motor A Control
const int engineSpeedB = 10;
const int engineDirectionB = 12;

// Motor B Control
const int engineSpeedA = 11;
const int engineDirectionA = 13;

// Motor control value from master
byte masterMotorValue = 0;

// On receive from Master board
void receiveEvent(int numOfBytes) {
  masterMotorValue = Wire.read();
}

/* MOTOR CONTROL VALUES - MINION
 * 0 - stop
 * 1 - forward
 * 2 - backward
 * 3 - left
 * 4 - right
*/

// Control the motors with official code structure from manufacturer
void Motor(char mot, char dir, int speed) {
  int newSpeed; // Remap the speed from range 0-100 to 27-255
  if (speed == 0) { // Don't remap 0
    newSpeed = 0;
  }
  else {
    newSpeed = map(speed, 1, 100, 27, 255); // 27 is min speed to move
  }

  switch (mot) {
    case 'A':   // Controlling Motor A
      if (dir == 'F') {
        digitalWrite(engineDirectionA, HIGH);
      }
      
      else if (dir == 'R') {
        digitalWrite(engineDirectionA, LOW);
      }
      
      analogWrite(engineSpeedA, newSpeed);
      break;

    case 'B':   // Controlling Motor B
      if (dir == 'F') {
        digitalWrite(engineDirectionB, LOW);
      }
      
      else if (dir == 'R') {
        digitalWrite(engineDirectionB, HIGH);
      }
      
      analogWrite(engineSpeedB, newSpeed);
      break;

    case 'C':  // Controlling Both Motors
      if (dir == 'F') {
        digitalWrite(engineDirectionA, HIGH);
        digitalWrite(engineDirectionB, LOW);
      }
      
      else if (dir == 'R') {
        digitalWrite(engineDirectionA, LOW);
        digitalWrite(engineDirectionB, HIGH);
      }
      
      analogWrite(engineSpeedA, newSpeed);
      analogWrite(engineSpeedB, newSpeed);
      break;
  }
}

void setup() {
  Serial.begin(9600); // Set to 9600 for debugging
  Wire.begin(8); // Connect to master under ID: 8 as minion

  Wire.onReceive(receiveEvent); // Call receiveEvent upon input from master

  pinMode(engineSpeedA, OUTPUT); // Set motor pinModes
  pinMode(engineSpeedB, OUTPUT);
  pinMode(engineDirectionA, OUTPUT);
  pinMode(engineDirectionB, OUTPUT);

  // Add tasks
  SoftTimer.add(&task1); // Motors
}

void motorTask(Task* me) {
  Serial.println(masterMotorValue);
  switch (masterMotorValue) { // Control motors according to value from master
    case 0: // Stop
      Motor('C', 'F', 0);
      break;
      
    case 1: // Forward
      Motor('C', 'F', 25);
      break;
      
    case 2: // Backwards
      Motor('C', 'R', 25);
      break;
      
    case 3: // Left
      Motor('A', 'R', 25);
      Motor('B', 'F', 25);
      break;
      
    case 4: // Right
      Motor('A', 'F', 25);
      Motor('B', 'R', 25);
      break;
  }
}
