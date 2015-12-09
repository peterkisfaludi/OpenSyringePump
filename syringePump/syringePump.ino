// Controls a stepper motor via an LCD keypad shield.
// Accepts triggers and serial commands.

// Serial commands:
// Send a number, e.g. "100", to set bolus size.
// Send a "+" to push that size bolus.

#include "AccelStepper.h"

/* -- Constants -- */
#define SYRINGE_VOLUME_ML 140.0
#define SYRINGE_BARREL_LENGTH_MM 123.44

#define THREADED_ROD_PITCH 1.25
#define STEPS_PER_REVOLUTION 200.0

#define RAMP_UP_TIME_SEC 0.4

unsigned long stepsPerML = (STEPS_PER_REVOLUTION * SYRINGE_BARREL_LENGTH_MM) / (SYRINGE_VOLUME_ML * THREADED_ROD_PITCH );

/* -- Pin definitions -- */
int motorStepPin = 2;
int motorDirPin = 3;

int motorMS1Pin = 4;
int motorMS2Pin = 5;
int motorMS3Pin = 6;

AccelStepper stepper(1, motorStepPin, motorDirPin);

/* -- Default Parameters -- */
float mLUsed = 0.0;

float mLBolus = 10.0; // pump volume

const float minStepsPerSec = 1400.0;
const float maxStepsPerSec = 7053.0;
float reqStepsPerSec = maxStepsPerSec;
const float motorAccel = (maxStepsPerSec - minStepsPerSec) / RAMP_UP_TIME_SEC;

//serial
String serialStr = "";
boolean serialStrReady = false;

// set full stepping mode
void setFullStepMode() {
  digitalWrite(motorMS1Pin, LOW);
  digitalWrite(motorMS2Pin, LOW);
  digitalWrite(motorMS3Pin, LOW);
}

void setup() {

  /* Motor Setup */
  pinMode(motorStepPin, OUTPUT);
  pinMode(motorDirPin, OUTPUT);

  pinMode(motorMS1Pin, OUTPUT);
  pinMode(motorMS2Pin, OUTPUT);
  pinMode(motorMS3Pin, OUTPUT);

  setFullStepMode();

  stepper.setMaxSpeed(reqStepsPerSec);
  stepper.setAcceleration(motorAccel);

  /* Serial setup */
  //Note that serial commands must be terminated with a newline
  //to be processed. Check this setting in your serial monitor if
  //serial commands aren't doing anything.
  Serial.begin(9600);

  Serial.print("reqStepsPerSec=");
  Serial.print(reqStepsPerSec);
  Serial.println("");

  Serial.print("motorAccel=");
  Serial.print(motorAccel);
  Serial.println("");

}

void loop() {

  //check serial port for new commands
  readSerial();
  if (serialStrReady) {
    processSerial();
  }
}

void readSerial() {
  //pulls in characters from serial port as they arrive
  //builds serialStr and sets ready flag when newline is found
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      serialStrReady = true;
    }
    else {
      serialStr += inChar;
    }
  }
}

void processSerial() {
  //process serial commands as they are read in

  static long posSteps = 0;

  if (serialStr.equals("+")) {
    goTo(posSteps);
  }
  else {
    posSteps = serialStr.toInt();
    Serial.print("posSteps=");
    Serial.print(posSteps);
    Serial.println("");
  }

  serialStrReady = false;
  serialStr = "";
}

unsigned long mstimestart;
unsigned long mstimeend;

void goTo(long posSteps) {
  Serial.print("go to position [steps] =");
  Serial.print(posSteps);
  Serial.println("");
  mstimestart = micros();
  stepper.runToNewPosition(posSteps);
  mstimeend = micros();
  Serial.println("stepper ready");
  Serial.print("time elapsed [us]= ");
  Serial.print(mstimeend - mstimestart);
  Serial.println("");
}
