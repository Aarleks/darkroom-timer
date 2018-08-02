//From bildr article: http://bildr.org/2012/08/rotary-encoder-arduino/

//We always have to include the library
#include "LedControl.h"

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

//VARIABLES
int butDebounce = 20; // general variable for button debouncing

byte previousFocus = LOW;// For determining Focus button change state
long lastFocusTime = 0; // For the debounce of the Focus button
long debounce = 200;//for the Focus and Start buttons debounce
byte focusState;
byte enlargerOn = 0;

int brightness = 8; // Initial brightness value
byte modeState = LOW;// state of the Mode button for cycling through Modes
byte modePrevious = HIGH;
unsigned long pressTime = 0; // For the debounce of the Mode button
byte generalMode = 3;// For the switch... case argument

//ROTARY ENCODER VARIABLES
volatile int lastEncoded = 0;
volatile long encoderValue = 0;

long lastencoderValue = 0;

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

  pinMode(encoderPin1, INPUT); 
  pinMode(encoderPin2, INPUT);
  pinMode(encoderSwitchPin, INPUT);
  pinMode(focusLed, OUTPUT);
  pinMode(focusButton, INPUT);
  pinMode(modeButton, INPUT);

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
  modeState = digitalRead(modeButton);
  if(modeState && modeState != modePrevious){
    if(generalMode < 3) // Check to see if the generalMode is within a range of 0 - 3,  anything over that will reset count back to 0.  
      generalMode += 1; 
    else
      generalMode = 0;

    Serial.println(generalMode);
  } 
  modePrevious = modeState;

  //set the state of the focus button
  focusState = digitalRead(focusButton);
  //Serial.println(focusState);
  if(focusState && focusState != previousFocus){
    if(enlargerOn > 0)
      enlargerOn = 0;
    else
      enlargerOn = 1;
    
    Serial.println(enlargerOn);
  }
  previousFocus = focusState;


  if(digitalRead(encoderSwitchPin)){
    //button is not being pushed
  }
  else{
    //button is being pushed
    encoderValue = 0;
  }
  
  

  /*
  if(focusState == HIGH && previousFocus == LOW && millis() - lastFocusTime > debounce) {
    if(enlargerOn == HIGH){
      enlargerOn = LOW; 
      butDebounce = 0;
      // write home message on display
    } 
    else {
       enlargerOn = HIGH;
       butDebounce = millis(); 
       //write "Focus on" to the display
    }
    lastFocusTime = millis();
    previousFocus == focusState;
  }
  */
  digitalWrite(enlargerLamp, enlargerOn);
  digitalWrite(focusLed, enlargerOn);


  //Write stuff to the display

  brightness = constrain(encoderValue, 0, 15);
  lc.setIntensity(0, brightness);

  lc.setChar(0,7,'F',false);
  lc.setChar(0,6,'a',false);
  lc.setRow(0,5,0x05);
  lc.setChar(0,4,'d',false);
  lc.setRow(0,3,0x1c);
  lc.setRow(0,2,B00010000);
  lc.setChar(0,1,'n',false);
  lc.setChar(0,0,'o',false);
 
  //Serial.println(encoderValue);
  //delay(100); //just here to slow down the output, and show it will work  even during a delay
}


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

