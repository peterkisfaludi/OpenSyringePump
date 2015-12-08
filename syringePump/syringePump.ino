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
#define MICROSTEPS_PER_STEP 16.0
#define MICROSTEPS_PER_REVOLUTION (STEPS_PER_REVOLUTION * MICROSTEPS_PER_STEP)

#define RAMP_UP_TIME_SEC 0.4

unsigned long ustepsPerMM = MICROSTEPS_PER_REVOLUTION / THREADED_ROD_PITCH;
unsigned long ustepsPerML = (MICROSTEPS_PER_REVOLUTION * SYRINGE_BARREL_LENGTH_MM) / (SYRINGE_VOLUME_ML * THREADED_ROD_PITCH );

/* -- Pin definitions -- */
int motorStepPin = 2;
int motorDirPin = 3;
AccelStepper stepper(1, motorStepPin, motorDirPin);

/* -- Enums and constants -- */
enum {PUSH, PULL}; //syringe movement direction
enum {MAIN, SPEED, VOLUME, SYMCYCLES, PREHEAT, RUN}; //UI states

/* -- Default Parameters -- */
float mLUsed = 0.0;

float mLBolus = 10.0; // pump volume

const float minUstepsPerSec = 1400.0 * MICROSTEPS_PER_STEP;
const float maxUstepsPerSec = 7053.0 * MICROSTEPS_PER_STEP;
float reqUstepsPerSec = maxUstepsPerSec;
const float motorAccel = (maxUstepsPerSec - minUstepsPerSec) / RAMP_UP_TIME_SEC;

//serial
String serialStr = "";
boolean serialStrReady = false;

void setup() {

  /* Motor Setup */
  pinMode(motorDirPin, OUTPUT);
  pinMode(motorStepPin, OUTPUT);

  /* Serial setup */
  //Note that serial commands must be terminated with a newline
  //to be processed. Check this setting in your serial monitor if
  //serial commands aren't doing anything.
  Serial.begin(9600); //Note that your serial connection must be set to 57600 to work!
  delay(5000);

  Serial.print("minUstepsPerSec=");
  Serial.print(minUstepsPerSec);
  Serial.println("");

  Serial.print("maxUstepsPerSec=");
  Serial.print(maxUstepsPerSec);
  Serial.println("");

  Serial.print("reqUstepsPerSec=");
  Serial.print(reqUstepsPerSec);
  Serial.println("");

  Serial.print("motorAccel=");
  Serial.print(motorAccel);
  Serial.println("");

  stepper.setMaxSpeed(maxUstepsPerSec);
  stepper.setSpeed(reqUstepsPerSec);
  stepper.setAcceleration(motorAccel);
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

  static long posUsteps = 0;

  if (serialStr.equals("+")) {
    goTo(posUsteps);
  }
  else if (serialStr.toInt() != 0) {
    posUsteps = serialStr.toInt();
    Serial.print("posUsteps=");
    Serial.print(posUsteps);
    Serial.println("");
  }
  else {
    Serial.println("Invalid command");
  }

  serialStrReady = false;
  serialStr = "";
}

unsigned long mstimestart;
unsigned long mstimeend;

void goTo(unsigned long posUsteps) {
  Serial.print("go to position [usteps] =");
  Serial.print(posUsteps);
  Serial.println("");

  stepper.moveTo(posUsteps);

  mstimestart = micros();
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  mstimeend = micros();
  Serial.println("stepper ready");
  Serial.print("time elapsed [us]= ");
  Serial.print(mstimeend - mstimestart);
  Serial.println("");
}
