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

int relay1Pin = 22;
int relay2Pin = 23;
int relay3Pin = 24;

/* -- Enums and constants -- */
enum {PUSH, PULL}; //syringe movement direction
enum {MAIN, SPEED, VOLUME, SYMCYCLES, PREHEAT, RUN}; //UI states

/* -- Default Parameters -- */
float mLUsed = 0.0;

const float minMlBolus = 10.0;
const float maxMlBolus = 120.0;
float mLBolus = minMlBolus; // pump volume
const float minuStepsPerSec = 1400.0 * MICROSTEPS_PER_STEP;
const float maxuStepsPerSec = 7053.0 * MICROSTEPS_PER_STEP;
float ustepsPerSec = minuStepsPerSec;
int symcycles = 1;
float preHeatTimeSec = 1.0;

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
  Serial.println("1 - Select pump speed [mL/sec]");
  Serial.println("2 - Select pump volume [mL]");
  Serial.println("3 - Select number of symmetrical cycles [positive integer]");
  Serial.println("4 - Select pre-heat time [seconds]");
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

enum {ON, OFF};
void relayControl(int relayNum, int onoff) {
  int relayPin;
  int highlow;

  switch (relayNum) {
    case 1:
      {
        relayPin = relay1Pin;
        break;
      }
    case 2:
      {
        relayPin = relay2Pin;
        break;
      }
    case 3:
      {
        relayPin = relay3Pin;
        break;
      }
    default:
      {
        return;
      }
  }

  switch (onoff) {
    case ON:
      {
        highlow = HIGH;
        break;
      }
    case OFF:
      {
        highlow = LOW;
        break;
      }
    default:
      return;
  }

  digitalWrite(relayPin, onoff);
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
                Serial.println("Enter desired pump speed [mL / sec]");
                break;
              }
            case 2:
              {
                uiState = VOLUME;
                Serial.println("Enter desired pump volume [mL]");
                break;
              }
            case 3:
              {
                uiState = SYMCYCLES;
                Serial.println("Enter desired symmetrical cycles [poitive integer]");
                break;
              }
            case 4:
              {
                uiState = PREHEAT;
                Serial.println("Enter desired preheat time [seconds]");
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
        float tmp = ustepsPerML * serialStr.toFloat();
        if (tmp >= minuStepsPerSec && tmp <= maxuStepsPerSec) {
          ustepsPerSec = tmp;          
        } else {
          Serial.println("invalid number");
        }
        Serial.print("ustepsPerSec = ");
        Serial.println(ustepsPerSec);
        uiState = MAIN;
        showMain();
        break;
      }

    case VOLUME:
      {
        float tmp = serialStr.toFloat();
        if (tmp >= minMlBolus && tmp <= maxMlBolus) {
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
        //TODO change motor speed to ustepsPerSec      

        // TODO tell motor to take usteps number of steps
        unsigned long usteps = mLBolus * ustepsPerML;
        
        for (unsigned long i = 0; i < symcycles; i++) {
          Serial.print("Starting cycle ");
          Serial.println(i+1);
          //Start with all relays off
          relayControl(1, OFF);
          relayControl(2, OFF);
          relayControl(3, OFF);

          //Relay 1 turns on
          relayControl(1, ON);

          //Relay 3 turns on for selected pre-heat time
          relayControl(3, ON);

          //pre-heat
          Serial.println("Preheat start ");
          delay(1000.0 * preHeatTimeSec);

          //WITHDRAW
          Serial.println("WITHDRAW start ");
          
          //TODO do withdraw

          //Relay 3 turns off as soon as the stepper stops moving.
          relayControl(3, OFF);

          //Relay 1 turns off.
          relayControl(1, OFF);

          //Relay 2 turns on.
          relayControl(2, ON);

          //INFUSE
          Serial.println("INFUSE start");
          //TODO do infuse

          //Pause for 1 second.
          delay(1000);

          //Relay 2 turns off.
          relayControl(2, OFF);
        }

        uiState = MAIN;
        showMain();
        break;
      }
  }

  serialStrReady = false;
  serialStr = "";
}

