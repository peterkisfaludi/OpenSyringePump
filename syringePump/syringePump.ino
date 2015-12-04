// Controls a stepper motor via an LCD keypad shield.
// Accepts triggers and serial commands.

// Serial commands:
// Set serial baud rate to 57600 and terminate commands with newlines.
// Send a number, e.g. "100", to set bolus size.
// Send a "+" to push that size bolus.
// Send a "-" to pull that size bolus.

/* -- Constants -- */
#define SYRINGE_VOLUME_ML 140.0
#define SYRINGE_BARREL_LENGTH_MM 123.44

#define THREADED_ROD_PITCH 1.25
#define STEPS_PER_REVOLUTION 200.0
#define MICROSTEPS_PER_STEP 16.0
#define MICROSTEPS_PER_REVOLUTION (STEPS_PER_REVOLUTION * MICROSTEPS_PER_STEP)

#define RAMP_UP_TIME_SEC 0.4

long ustepsPerMM = MICROSTEPS_PER_REVOLUTION / THREADED_ROD_PITCH;
long ustepsPerML = (MICROSTEPS_PER_REVOLUTION * SYRINGE_BARREL_LENGTH_MM) / (SYRINGE_VOLUME_ML * THREADED_ROD_PITCH );

/* -- Pin definitions -- */
int motorStepPin = 2;
int motorDirPin = 3;

int motorMS1Pin = 4;
int motorMS2Pin = 5;
int motorMS3Pin = 6;

int motorEnablePin = 7;

/* -- Enums and constants -- */
enum {PUSH, PULL}; //syringe movement direction
enum {MAIN, SPEED, VOLUME, SYMCYCLES, PREHEAT, RUN}; //UI states

/* -- Default Parameters -- */
float mLUsed = 0.0;

float mLBolus = 10.0; // pump volume

const float minStepsPerSec = 1400.0;
const float maxStepsPerSec = 7053.0;
float stepsPerSec = minStepsPerSec;
long symcycles = 1;
float preHeatTimeSec = 1.0;

long stepperPos = 0; //in microsteps

//menu stuff
int uiState = MAIN;

//serial
String serialStr = "";
boolean serialStrReady = false;

// serial UI
/*
Software UI operation:

1. User selects pump speed (mL/s)
note: pump speed is directly related to stepper motor step frequency (steps/s)
2. User selects pump volume (mL)
note: pump volume is directly related to total number of stepper motor steps (# steps)
3. User selects number of symmetrical cycles (C)
4. User selects pre-heat time (X seconds)
5. User selects RUN, and program starts.
*/
void showMain() {
  Serial.println("Main menu, choose an option from below:");
  Serial.println("1 - Select pump speed");
  Serial.println("2 - Select pump volume");
  Serial.println("3 - Select number of symmetrical cycles");
  Serial.println("4 - Select pre-heat time");
  Serial.println("5 - RUN");
  Serial.println("6 - Show main menu");
}

// set full stepping mode
void setFullStepMode() {
  digitalWrite(motorMS1Pin, LOW);
  digitalWrite(motorMS2Pin, LOW);
  digitalWrite(motorMS3Pin, LOW);
}

// set 1/16 microstepping mode
void set16uStepMode() {
  digitalWrite(motorMS1Pin, HIGH);
  digitalWrite(motorMS2Pin, HIGH);
  digitalWrite(motorMS3Pin, HIGH);
}

void setup() {

  /* Motor Setup */
  pinMode(motorDirPin, OUTPUT);
  pinMode(motorStepPin, OUTPUT);

  pinMode(motorMS1Pin, OUTPUT);
  pinMode(motorMS2Pin, OUTPUT);
  pinMode(motorMS3Pin, OUTPUT);

  pinMode(motorEnablePin, OUTPUT);

  set16uStepMode();

  /* Serial setup */
  //Note that serial commands must be terminated with a newline
  //to be processed. Check this setting in your serial monitor if
  //serial commands aren't doing anything.
  Serial.begin(9600); //Note that your serial connection must be set to 57600 to work!

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

void showParams() {
  Serial.println("current parameters");
  Serial.print("Bolus [mL] = ");
  Serial.println(mLBolus);
  Serial.print("Bolus [mL] = ");
  Serial.println(mLBolus);
}


void processSerial() {
  //process serial commands as they are read in

  static long usteps = 0;

  if (serialStr.equals("+")) {
    Serial.print("pushing by ");
    Serial.print(usteps);
    Serial.print(" usteps");
    Serial.println("");
    turn(PUSH, usteps);
  }
  else if (serialStr.equals("-")) {
    turn(PULL, usteps);
  }
  else if (serialStr.toInt() != 0) {
    usteps = serialStr.toInt();
    Serial.print("usteps=");
    Serial.print(usteps);
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
void turn(int direction,long usteps) {
  Serial.print("turn called with usteps=");
  Serial.print(usteps);
  Serial.print(" dir= ");
  Serial.print(direction);
  Serial.println("");

  //test millis funtion
  mstimestart = millis();
  delay(400);
  mstimeend=millis();
  
  Serial.print("time start= ");
  Serial.print(mstimestart);
  Serial.println("");

  Serial.print("time elapsed= ");
  Serial.print(mstimeend-mstimestart);
  Serial.println("");
  
  Serial.print("time end= ");
  Serial.print(mstimeend);
  Serial.println("");
  
  
  if (direction == PUSH) {
    digitalWrite(motorDirPin, HIGH);
  }
  else if (direction == PULL) {
    digitalWrite(motorDirPin, LOW);
  }

  float minUstepsPerSec =  minStepsPerSec* MICROSTEPS_PER_STEP;
  float maxUstepsPerSec =  maxStepsPerSec* MICROSTEPS_PER_STEP;

  // a = (v_end - v0) / t
  float accelUsteps = (maxUstepsPerSec - minUstepsPerSec) / RAMP_UP_TIME_SEC;

  //v0 = 1400
  float actualUstepsPerSec = minUstepsPerSec;
  float t = 0.0;

  float usDelay;
  //ramping up speed
  Serial.print("ramping up speed");
  Serial.println(""); 

  while ( (actualUstepsPerSec < maxUstepsPerSec) && (usteps > 0L) ) {
    usteps--;
    usDelay = (1000000.0 / actualUstepsPerSec) / 2.0;
    
    digitalWrite(motorStepPin, HIGH);
    delayMicroseconds(usDelay);

    digitalWrite(motorStepPin, LOW);
    delayMicroseconds(usDelay);

    t += (usDelay * 2.0) / 1000000.0;

    //v = v0 + a*t
    actualUstepsPerSec = minUstepsPerSec + accelUsteps * t;
    if (actualUstepsPerSec > maxUstepsPerSec) {
      actualUstepsPerSec = maxUstepsPerSec;
    }

    /*
    Serial.print(" usDelay= ");
    Serial.print(usDelay);
    Serial.println("");  
    Serial.print("t[ms] = ");
    Serial.print(t*1000000.0);
    Serial.print(" actualUstepsPerSec = ");
    Serial.print(actualUstepsPerSec);
    Serial.print(" usteps remaining = ");
    Serial.print(usteps);
    Serial.println("");
    */
  }
   
    Serial.print(" usDelay= ");
    Serial.print(usDelay);
    Serial.println("");  
    Serial.print("t[ms] = ");
    Serial.print(t*1000000.0);
    Serial.print(" actualUstepsPerSec = ");
    Serial.print(actualUstepsPerSec);
    Serial.print(" usteps remaining = ");
    Serial.print(usteps);
    Serial.println("");
   

  // keep max speed till end (only if there are steps left
  Serial.println("holding speed");
  usDelay = (1000000.0 / maxUstepsPerSec) / 2.0;
  while (usteps > 0) {
    Serial.print(" usteps remaining = ");
    Serial.print(usteps);
    Serial.println("");

    usteps--;
    digitalWrite(motorStepPin, HIGH);
    delayMicroseconds(usDelay);

    digitalWrite(motorStepPin, LOW);
    delayMicroseconds(usDelay);
  }
}
