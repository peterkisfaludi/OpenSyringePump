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

//#define SPEED_MICROSECONDS_DELAY 100 //longer delay = lower speed
#define RAMP_UP_TIME 0.4 

long ustepsPerMM = MICROSTEPS_PER_STEP * STEPS_PER_REVOLUTION / THREADED_ROD_PITCH;
long ustepsPerML = (MICROSTEPS_PER_STEP * STEPS_PER_REVOLUTION * SYRINGE_BARREL_LENGTH_MM) / (SYRINGE_VOLUME_ML * THREADED_ROD_PITCH );

/* -- Pin definitions -- */
//TODO: check netlist with Mike
int motorDirPin = 2;
int motorStepPin = 3;

int motorMS1Pin = 4;
int motorMS2Pin = 5;
int motorMS3Pin = 6;

/* -- Enums and constants -- */
enum{PUSH,PULL}; //syringe movement direction
enum{MAIN, BOLUS_MENU}; //UI states

/* -- Default Parameters -- */
float mLUsed = 0.0;
float mLBolus = 5.0;

long stepperPos = 0; //in microsteps
char charBuf[16];
        
//menu stuff
int uiState = MAIN;

//serial
String serialStr = "";
boolean serialStrReady = false;

// set full stepping mode
void setFullStepMode(){
  digitalWrite(motorMS1Pin,LOW);
  digitalWrite(motorMS2Pin,LOW);
  digitalWrite(motorMS3Pin,LOW);
}

// set 1/16 microstepping mode
void set16uStepMode(){
  digitalWrite(motorMS1Pin,HIGH);
  digitalWrite(motorMS2Pin,HIGH);
  digitalWrite(motorMS3Pin,HIGH);
}

void setup(){
  
  /* Motor Setup */ 
  pinMode(motorDirPin, OUTPUT);
  pinMode(motorStepPin, OUTPUT);
  
  pinMode(motorMS1Pin, OUTPUT);
  pinMode(motorMS2Pin, OUTPUT);
  pinMode(motorMS3Pin, OUTPUT);
  
  setFullStepMode();
    
  /* Serial setup */
  //Note that serial commands must be terminated with a newline
  //to be processed. Check this setting in your serial monitor if 
  //serial commands aren't doing anything.
  Serial.begin(57600); //Note that your serial connection must be set to 57600 to work!
}

void loop(){
  
  //check serial port for new commands
  readSerial();
	if(serialStrReady){
		processSerial();
	}
}

void readSerial(){
		//pulls in characters from serial port as they arrive
		//builds serialStr and sets ready flag when newline is found
		while (Serial.available()) {
			char inChar = (char)Serial.read(); 
			if (inChar == '\n') {
				serialStrReady = true;
			} 
      else{
			  serialStr += inChar;
      }
		}
}

//TODO update as per spec
void processSerial(){
	//process serial commands as they are read in
	if(serialStr.equals("+")){
		bolus(PUSH);
	}
	else if(serialStr.equals("-")){
		bolus(PULL);
	}
  else if(serialStr.toInt() != 0){
    int uLbolus = serialStr.toInt();
    mLBolus = (float)uLbolus / 1000.0;
  }
  else{
     Serial.write("Invalid command: ["); 
     char buf[40];
     serialStr.toCharArray(buf, 40);
     Serial.write(buf); 
     Serial.write("]\n"); 
  }
  serialStrReady = false;
  serialStr = "";
}

void bolus(int direction){
  //Move stepper. Will not return until stepper is done moving.        
  
	//change units to steps
	long steps = (mLBolus * ustepsPerML);
	if(direction == PUSH){
        digitalWrite(motorDirPin, HIGH);
		mLUsed += mLBolus;
	}
	else if(direction == PULL){
        digitalWrite(motorDirPin, LOW);
		if((mLUsed-mLBolus) > 0){
			mLUsed -= mLBolus;
		}
		else{
			mLUsed = 0;
		}
	}	
      
      //TODO user provides max steps / sec parameter.  [min: 1400 steps/s; max: 7053 steps/s]
      // for now, assume 7053 is max
      
      //v0 = 1400
      float minStepsPerSec = 1400.0;
      //v_end = 7053.0
      float maxStepsPerSec = 7053.0;
      
      //enable microstepping
      set16uStepMode();
      
      // a = (v_end - v0) / t
      float accelSteps = (maxStepsPerSec - minStepsPerSec) / RAMP_UP_TIME;
      
      //v0 = 1400
      float actualStepsPerSec = minStepsPerSec;
      float t=0.0;
      
      //ramping up speed
      while(actualStepsPerSec < maxStepsPerSec){
        float usDelay = (1000000.0 / actualStepsPerSec)/2;
        digitalWrite(motorStepPin, HIGH); 
        delayMicroseconds(usDelay); 
    
        digitalWrite(motorStepPin, LOW); 
        delayMicroseconds(usDelay);
        
        t += usDelay*2;
        //v = v0 + a*t
        actualStepsPerSec += minStepsPerSec + accelSteps*t;
      }      

      //TODO keep speed
      /*
      for(long i=0; i < steps; i++){ 
        digitalWrite(motorStepPin, HIGH); 
        delayMicroseconds(usDelay); 
    
        digitalWrite(motorStepPin, LOW); 
        delayMicroseconds(usDelay); 
      }
      */
}

String decToString(float decNumber){
	//not a general use converter! Just good for the numbers we're working with here.
	int wholePart = decNumber; //truncate
	int decPart = round(abs(decNumber*1000)-abs(wholePart*1000)); //3 decimal places
        String strZeros = String("");
        if(decPart < 10){
          strZeros = String("00");
        }  
        else if(decPart < 100){
          strZeros = String("0");
        }
	return String(wholePart) + String('.') + strZeros + String(decPart);
}
