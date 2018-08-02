//From bildr article: http://bildr.org/2012/08/rotary-encoder-arduino/

//We always have to include the library
#include "LedControl.h"
#include "math.h"

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(12,11,10,1);

//these pins cannot be changed 2/3 are special pins
int encoderPin1 = 3;
int encoderPin2 = 2;
int encoderSwitchPin = 4; //push button switch
int enlargerLamp = 5;
int focusLed = 7;
int focusButton = 6;
int modeButton = 8;
int startButton = 9;
int startLed = 13;

//VARIABLES
boolean focusSwitch = true; // For indicating actuation of the Focus Button
boolean startSwitch = true; // For indicating actuation of the Start Button
boolean rotarySwitch = true; // For indicating actuation of the Rotary Encoder Pushbutton
byte previousFocus = LOW; // For determining Focus button change state
byte previousStart = HIGH; // For determining Start button change state
unsigned long currentMillis; // For timed operations
unsigned long previousMillis; // For timed operations
unsigned long interval = 3000; // For timed operations
boolean homeDisplay = true; // For display string cycling in Home mode
byte focusState;
byte startState;
int brightness = 8; // Initial brightness value
byte modeState = LOW; // state of the Mode button for cycling through Modes
byte modePrevious = HIGH;
byte generalMode = 3; // For the switch... case argument
float timerFStop = 0.0; // Default value for f-stop
int timerSeconds; // Calculated from timerFstop using fstop2TensSeconds()
//boolean rotaryState = true; // For switching the rotary encoder on and off
boolean previousRotary = true; // For switching the rotary encoder on and off
boolean rotaryEvent = true; // For rotary encoder pushbutton events

// Time Display Variables

int rightLength;
int timeStart = 999;
int displayTime;
unsigned long timerDecrement;

// F-stop Display Varialbes

int leftLength;



//long debounce = 200;//for the Focus and Start buttons debounce
//byte enlargerOn = 1;
//int butDebounce = 20; // general variable for button debouncing
//unsigned long pressTime = 0; // For the debounce of the Mode button


//ROTARY ENCODER VARIABLES
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
volatile long tempEncoder = 0;

//long lastencoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;
// END VARIABLES
//------------------------------------------------------------------------



void setup() {
  Serial.begin (9600);
  // Bring the display out of shutdown
  lc.shutdown(0,false);
  // and clear the display
  lc.clearDisplay(0);
  // set brightness

  pinMode(encoderPin1, INPUT); 
  pinMode(encoderPin2, INPUT);
  pinMode(encoderSwitchPin, INPUT);
  pinMode(focusLed, OUTPUT);
  pinMode(focusButton, INPUT);
  pinMode(modeButton, INPUT);
  pinMode(startLed, OUTPUT);
  pinMode(startButton, INPUT);

  modeState = digitalRead(modeButton); //read the initial state of the button

  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  digitalWrite(encoderSwitchPin, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3) 
  attachInterrupt(0, updateEncoder, CHANGE); 
  attachInterrupt(1, updateEncoder, CHANGE);

}

void loop(){ 
  //void checkMode(){
  lc.setIntensity(0, brightness);
  modeState = digitalRead(modeButton);
  if(modeState && modeState != modePrevious){
    if(generalMode < 3){ // Check to see if the generalMode is within a range of 0 - 3,  anything over that will reset count back to 0.  
      generalMode += 1;
      encoderValue = 0;
      lc.clearDisplay(0);
    }
    else{
      generalMode = 0;
      lc.clearDisplay(0);
    }

    Serial.println(generalMode);
  } 
  modePrevious = modeState;

  timerMode();

}

/////// ENLARGER FUNCTIONS  //////////

// THE INTERRUPT
void updateEncoder(){
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

  lastEncoded = encoded; //store this value for next time
}

void timerMode(){

  rotaryPBEvent();
  if(rotaryEvent == 1){
    timerFStop = constrain(encoderValue*10, 0, 900);
    displayTime = timerSeconds;
    lc.setChar(0,4,' ', false);
  }
  if(rotaryEvent == 0){
    lc.setChar(0,4,' ', true);
    expose();
  }
  
  timerSeconds = fstop2TensSeconds(timerFStop);
  leftDisplayText(String(timerFStop/100, 1));

  // Run the countdown timer function to switch the enlarger on for the selected time
  countdownTimer();
  Serial.println(displayTime);
  //display time as it counts down
  rightDisplayText(String(displayTime));
  // Switch the start LED and enlarger lamp on
  digitalWrite(startLed, startSwitch);
  digitalWrite(enlargerLamp, startSwitch);
  
}

void focus(){
  //set the state of the focus button
  focusState = digitalRead(focusButton);
  if(focusState && focusState != previousFocus){
    if(focusSwitch > 0)
      focusSwitch = 0;
    else
      focusSwitch = 1;
  }
  previousFocus = focusState;
}

void expose(){
  //set the state of the start button
  startState = digitalRead(startButton);
  if(startState && startState != previousStart){
    startSwitch = !startSwitch;
    /*if(startSwitch > 0)
      startSwitch = 0;
    else
      startSwitch = 1;*/
  }
  previousStart = startState;
}

void rotaryPB(){
  if(digitalRead(encoderSwitchPin)){
    //button is not being pushed
    rotarySwitch = 1;
  }
  else{
    //button is being pushed
    rotarySwitch = 0;
  }
}

void rotaryPBEvent(){
  // event trigger based on pushing the rotary encoder pushbutton
  rotaryPB();
  if(rotarySwitch && rotarySwitch != previousRotary){
    rotaryEvent = !rotaryEvent;
  }
  previousRotary = rotarySwitch;
}

int fstop2TensSeconds(float FStop){
  float tensValue = 0;
  tensValue = round(10*pow(2,FStop/100));//adapted f-stop to time formula: t=2^(fstop), here time in 1/10th of sec, f-stop in 1/100th
  return tensValue;
}

void leftDisplayText(String text){
  String leftTempText = text;
  int leftDotPosition = leftTempText.indexOf('.');
  leftTempText.remove(leftDotPosition,1);
  leftLength = leftTempText.length();
  for(int i = 0; i < leftLength; i++){
    if(i == leftDotPosition - 1){
      lc.setChar(0, 7-i, leftTempText[i], true);
      }
    else{
      lc.setChar(0,7-i,leftTempText[i],false);
    }
    if(leftLength < 3){
      lc.setChar(0, 5,' ', false);
      //lc.setChar(0, 4,' ', false);
    }
  }
}

void rightDisplayText(String text){
  String rightTempText = text;
  rightLength = rightTempText.length();
  for(int o = 0; o < rightLength; o++){
    if(o ==  1){
      lc.setChar(0, 0+o, rightTempText[(rightLength - 1) - o], true);
      }
    else{
      lc.setChar(0,0+o,rightTempText[(rightLength - 1) - o],false);
    }
    if(rightLength < 2){
      lc.setChar(0, 1, '0', true);
    }
    if(rightLength < 3){
      lc.setChar(0, 2,' ', false);
    }
    if(rightLength < 4){
      lc.setChar(0, 3,' ', false);
    }
  }
}

void countdownTimer(){
  if(startSwitch == true){
    if(timeStart == 999){
      timeStart = timerSeconds;
      timerDecrement = millis();
    }
    else if(millis() - timerDecrement > 99){
      timerDecrement = millis();
      if(timeStart > 0){
        timeStart -= 1;
        displayTime = timeStart;
        }
      else if (timeStart == 0){
        startSwitch = false;
        timerDecrement = 0;
        timeStart = 999;
        }
    }
      
  }
}

