// Controls a stepper motor via an LCD keypad shield.
// Accepts triggers and serial commands.

// Serial commands:
// Set serial baud rate to 9600 and terminate commands with newlines.

/* -- Constants -- */
#define SYRINGE_VOLUME_ML 140.0
#define SYRINGE_BARREL_LENGTH_MM 123.44

#define THREADED_ROD_PITCH 1.25
#define STEPS_PER_REVOLUTION 200.0
#define MICROSTEPS_PER_STEP 16.0

#define RAMP_UP_TIME 0.4

long ustepsPerMM = MICROSTEPS_PER_STEP * STEPS_PER_REVOLUTION / THREADED_ROD_PITCH;
long ustepsPerML = (MICROSTEPS_PER_STEP * STEPS_PER_REVOLUTION * SYRINGE_BARREL_LENGTH_MM) / (SYRINGE_VOLUME_ML * THREADED_ROD_PITCH );

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
float mLBolus = 5.0; // pump volume
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

void setup() {

  Serial.begin(9600); //Note that your serial connection must be set to 9600 to work!
  showMain();
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

  switch (uiState) {
    case MAIN:
      {
        int choice = serialStr.toInt();
        if (choice != 0) {
          switch (choice) {
            case 1:
              {
                uiState = SPEED;
                Serial.println("Enter desired pump speed");
                break;
              }
            case 2:
              {
                uiState = VOLUME;
                Serial.println("Enter desired pump volume");
                break;
              }
            case 3:
              {
                uiState = SYMCYCLES;
                Serial.println("Enter desired symmetrical cycles");
                break;
              }
            case 4:
              {
                uiState = PREHEAT;
                Serial.println("Enter desired preheat time");
                break;
              }
            case 5:
              {
                uiState = RUN;
                processSerial();
                break;
              }
            case 6:
              {
                showMain();
                break;
              }
            default:
              {
                Serial.println("invalid choice");
                break;
              }
          }
        } else {
          Serial.println("invalid choice");
        }
        break;
      }

    case SPEED:
      {
        float tmp = serialStr.toFloat();
        if (tmp >= minStepsPerSec && tmp <= maxStepsPerSec) {
          stepsPerSec = tmp;
          //TODO change motor speed
        } else {
          Serial.println("invalid number");
        }
        Serial.print("stepsPerSec = ");
        Serial.println(stepsPerSec);
        uiState = MAIN;
        showMain();
        break;
      }

    case VOLUME:
      {
        float tmp = serialStr.toFloat();
        if (tmp > 0.0) {
          mLBolus = tmp;
        } else {
          Serial.println("invalid number");
        }
        Serial.print("mLBolus = ");
        Serial.println(mLBolus);
        uiState = MAIN;
        showMain();
        break;
      }

    case SYMCYCLES:
      {
        int tmp = serialStr.toInt();
        if (tmp > 0) {
          symcycles = tmp;
        } else {
          Serial.println("invalid number");
        }
        Serial.print("symcycles = ");
        Serial.println(symcycles);
        uiState = MAIN;
        showMain();
        break;
      }

    case PREHEAT:
      {
        float tmp = serialStr.toFloat();
        if (tmp > 0.0) {
          preHeatTimeSec = tmp;
        } else {
          Serial.println("invalid number");
        }
        Serial.print("preHeatTimeSec = ");
        Serial.println(preHeatTimeSec);
        uiState = MAIN;
        showMain();
        break;
      }
      
    case RUN:
      {
        //TODO do full cycle as per spec:
        /*
         * Relay 1 on, relay 3 on
         * WITHDRAW
         * Relay 3 off, relay 1 off, relay 2 on
         * INFUSE
         * PAUSE 1 sec
         * Relay 2 off
         *
         */

        unsigned long usteps = mLBolus * ustepsPerML;
        //TODO position motor based on usteps
        Serial.println("run start");
        Serial.println("run end");
        uiState = MAIN;
        showMain();
        break;
      }
  }

  serialStrReady = false;
  serialStr = "";
}
