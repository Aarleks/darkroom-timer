/*
 * F-STOP ENLARGER TIMER
 * 
 * This script runs the Albermarle St F-Stop timer on an Arduino Uno R3.
 * The script takes inputs from a rotary encoder and four momentary pushbuttons to set a time value based 
 * on Gene Nocton's f-stop timer concept. 
 * 
 * The script uses the LedControl.h library (available here: https://github.com/wayoda/LedControl) to run
 * a 2 x 4 digit 7-segment display unit controlled by the MAX7219 chip. 
 * 
 * The circuit operates a relat, to which the enlarger and mains power are connected. 
 * EXERCISE ALL POSSIBLE CAUTION WITH MAINS POWER!
 * 
 */

#include "LedControl.h"

// PINS IN USE ON THE ARDUINO UNO R3
int encoderPin2 = 2;      // This pin must be to the rotary encoder
int encoderPin1 = 3;      // This pin must be to the rotary encoder
int encoderSwitchPin = 4; // Rotary encoder pushbutton
int enlargerLamp = 5;     // Connection to relay
int focusButton = 6;      // Pushbutton for Focus 
int focusLed = 7;         // LED for indicating Focus button use
int modeButton = 8;       // Pushbutton for changing the timer moder
int startButton = 9;      // Pushbutton for Start
// pin 10 to MAX7219 CS blue
// pin 11 to MAX7219 CLK green
// pin 12 to MAX7219 DIN orange
int startLed = 13;        // LED for indicating Start button use

// Call the LedControl library and nominate the pins, last argument is the number of MAX7219 chips.
LedControl lc = LedControl(12,11,10,1);

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
byte focusState; // Whether the focus button is in on or off mode
byte startState; // Whether the start button is in on or off mode
int brightness = 8; // Initial brightness value
byte modeState = LOW; // state of the Mode button for cycling through Modes
byte modePrevious = HIGH; // What the previous state of the mode button was
byte generalMode = 3; // For the switch... case argument
float timerFStop = 0.0; // Default value for f-stop
int timerSeconds; // Calculated from timerFstop using fstop2TensSeconds()
//boolean rotaryState = true; // For switching the rotary encoder on and off
boolean previousRotary = true; // For switching the rotary encoder on and off
boolean rotaryEvent = true; // For rotary encoder pushbutton events
int stripState = 0; // For cycling through the sub-modes in the stripMode()
boolean previousStripRotary = true; // For cycling through the sub-modes in StripMode()
const int stripNumbers[] = {1,2,3,4,5,6,7,8,9,10}; // Possible values for the number of exposures
int numberStrips = 0; // For placing the number of strips to be worked with
int n = 0; // For manipulating rotary encoder choices in stripMode and fstopMode
const int stripStopIntervals[] = {10,20,25,33,50,66,75}; // Possible values for the number of exposures
int stripStops = 0; // For saving the number of strips in an exposure
float stripBase = 0.0; // For displaying the base time for stripMode()
int exposureNumber = 0; // For stepping through the exposure sequences in stripMode() and fstopMode()
int stripSeconds; // For the seconds variable to be fed to exposure()

// Time Display Variables
int rightLength; // Variable to take the length of the string for the right-hand display
int timeStart = 999;
int displayTime;
unsigned long timerDecrement;

// F-stop Display Varialbes
int leftLength;


//ROTARY ENCODER VARIABLES
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
long lastencoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;

// END VARIABLES
//------------------------------------------------------------------------


void setup() {
  lc.shutdown(0,false);   // Bring the display out of shutdown
  lc.clearDisplay(0);     // and clear the display

  // Bring the pins online
  pinMode(encoderPin1, INPUT); 
  pinMode(encoderPin2, INPUT);
  pinMode(encoderSwitchPin, INPUT);
  pinMode(enlargerLamp, OUTPUT);
  pinMode(focusLed, OUTPUT);
  pinMode(focusButton, INPUT);
  pinMode(modeButton, INPUT);
  pinMode(startButton, INPUT);
  pinMode(startLed, OUTPUT);

  Serial.begin(9600);

  modeState = digitalRead(modeButton); //read the initial state of the button

  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on
  digitalWrite(encoderSwitchPin, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3) 
  attachInterrupt(0, updateEncoder, CHANGE); 
  attachInterrupt(1, updateEncoder, CHANGE);

}

void loop() {
  // Which Mode am I in?
  // What does that Mode want me to do?
  modeState = digitalRead(modeButton);
  if(modeState && modeState != modePrevious){
    if(generalMode < 3){ // Check to see if the generalMode is within a range of 0 - 3,  anything over that will reset count back to 0.  
      generalMode += 1;
      encoderValue = 0;
    }
    else{
      generalMode = 0;
      encoderValue = 0;
    }

    //Serial.println(generalMode);
  } 
  modePrevious = modeState;

  // Switch modes
  switch (generalMode){

    case 0:
      homeMode();
      break;

    case 1:
      timerMode();
      break;

    case 2:
      stripMode();
      break;

    case 3:
      break;
  }
}





// THE INTERRUPT FOR THE ROTARY ENCODER //
void updateEncoder(){
  int MSB = digitalRead(encoderPin1);       //MSB = most significant bit
  int LSB = digitalRead(encoderPin2);       //LSB = least significant bit

  int encoded = (MSB << 1) |LSB;            //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded;  //adding it to the previous encoded value

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

  lastEncoded = encoded;                    //store this value for next time
}

//----------------------------------------------------------
////// MODES OF OPERATION //////
//----------------------------------------------------------

//// THE HOME MODE INCLUDING FOCUS LIGHT OPERATION ////
void homeMode(){
  
  focus(); // Focus light operation
  digitalWrite(enlargerLamp, focusSwitch);
  digitalWrite(focusLed, focusSwitch);
  rotaryPB();

  // display brightness reset
  if(rotarySwitch == 0){
    encoderValue = 0;
  }
  // display brightness adjust  
  brightness = constrain(encoderValue, 0, 15);
  lc.setIntensity(0, brightness);

  //Write stuff to the display
  currentMillis = millis();
  if(currentMillis - previousMillis > interval){
    previousMillis = currentMillis;
    homeDisplay = !homeDisplay;
    lc.clearDisplay(0);
  }
  if(homeDisplay){
    lc.setChar(0,7,'F',false);
    lc.setRow(0,6,1);
    lc.setChar(0,5,'5',false);
    //lc.setChar(0,4,'1',false);
    lc.setRow(0,4,7);
    lc.setChar(0,3,'o',false);
    lc.setChar(0,2,'p',false);
  }
  else{
    lc.setChar(0,7,'a',false);
    lc.setRow(0,6,0x05);
    lc.setChar(0,5,'d',false);
    lc.setRow(0,4,0x1c);
    lc.setRow(0,3,B00010000);
    lc.setChar(0,2,'n',false);
    lc.setChar(0,1,'o',false);
  }
}

//// PLAIN COUNTDOWN TIMER MODE, AS WITH NORMAL ENLARGER TIMERS ////
void timerMode(){

  rotaryPBEvent();
  focus();
  if(focusSwitch == 1){
    brightness = 0;
    lc.setIntensity(0, brightness);
  }
  digitalWrite(focusLed, focusSwitch);
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
  
  //display time as it counts down
  rightDisplayText(String(displayTime));
  // Switch the start LED and enlarger lamp on
  digitalWrite(startLed, startSwitch);
  digitalWrite(enlargerLamp, startSwitch);
  
}


//// THE TEST STRIP EXPOSURE MODE ////
void stripMode(){
  rotaryPBEvent();
  if(rotaryEvent != previousStripRotary){
    if(stripState < 3){ // Check to see if the stripState is within a range of 0 - 3,  anything over that will reset count back to 0.  
      stripState += 1;
      encoderValue = 0;
      n = 0;
      previousStripRotary = rotaryEvent;
    }
    else{
      stripState = 0;
      encoderValue = 0;
      n = 0;
      previousStripRotary = rotaryEvent;
    }
  }

  // Choose the number of strips to make
  if(stripState == 0){
    // rotary does no. strips
    timeStart = 999;
    n = constrain(encoderValue, 0, 9);
    numberStrips = stripNumbers[n];
    String tempo = "n ";
    tempo = tempo + numberStrips;
    leftDisplayText(tempo);
  }

  // Choose the f-stop increment for each strip
  else if(stripState == 1){
    // rotary does f-stop increment
    n = constrain(encoderValue, 0, 6);
    stripStops = stripStopIntervals[n];
    String tempo = "1 ";
    tempo = tempo + stripStops;
    leftDisplayText(tempo);
  }

  // Choose the base exposure time to begin with
  else if(stripState == 2){
    // rotary does base f-stop (time)
    // left display 'b'
    stripBase = constrain(encoderValue*10, 0, 800);
    String tempo = "b ";
    tempo = tempo + String(stripBase/100,1);
    leftDisplayText(tempo); 
    rightDisplayText(String(fstop2TensSeconds(stripBase)));
  }

  // Execute the strip exposure program (base + number@fstop increment)
  else if(stripState == 3){
    int stripSequence[numberStrips];
    int stripFStops[numberStrips];
    for(int i = 0; i < numberStrips; i++){
      if(i == 0){
        stripSequence[i] = fstop2TensSeconds(stripBase);
        stripFStops[i] = stripBase;
      }
      else{
        stripSequence[i] = fstop2TensSeconds(stripBase + (i * stripStops)) - fstop2TensSeconds(stripBase + ((i - 1) * stripStops));
        stripFStops[i] = stripBase + (i * stripStops);
      }
    }
    if(exposureNumber > numberStrips){
      exposureNumber = 0;
    }
    stripSeconds = stripSequence[exposureNumber];
    timerSeconds = stripSeconds;   
    expose();
    if(startSwitch == false){
      // left display has exposure number + f-stop
      // right display has corresponding time
      displayTime = timerSeconds;
    }
    countdownTimer();
    String leftStripText = String(exposureNumber + 1);
    float tata = stripFStops[exposureNumber];
    leftStripText += '-';
    leftStripText = leftStripText + String(tata/100,1);
    leftDisplayText(leftStripText);
    rightDisplayText(String(displayTime));
  }

  // Switch the Start LED and enlarger lamp on
  digitalWrite(startLed, startSwitch);
  digitalWrite(enlargerLamp, startSwitch);
}


/////////   INPUT FUNCTIONS - BUTTONS and ENCODERS     //////////

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
    }
    if(leftLength < 4){
      lc.setChar(0, 4,' ', false);
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
        if(exposureNumber == numberStrips - 1){
          exposureNumber = 0;
        }
        else if(exposureNumber < numberStrips - 1){
          exposureNumber += 1;
        }
        timeStart = 999;
      }
    }
  }
}


