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

int focusState = 0;
float potVal;
float test1 = 1.25;
const int analogInPin = A0;
const int enlargerLamp = 13;
const int focusButton = 4;
int brightness;
int enlargerOn = LOW;
int previous = LOW;
long lastTime = 0;
long debounce = 200;

void setup() {
    /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);

  pinMode(enlargerLamp, OUTPUT);
  pinMode(focusButton, INPUT);

}

void loop() {
  
  //set brightness of display
  potVal = analogRead(analogInPin);
  brightness = map(potVal, 0, 1023, 0, 15);
  lc.setIntensity(0, brightness);
  
  //set the state of the focus button
  focusState = digitalRead(focusButton);
  /*if(focusState == HIGH && !enlargerOn){
    enlargerOn = !enlargerOn;
  }
  if(enlargerOn){
    digitalWrite(enlargerLamp, HIGH);
  }*/
  if(focusState == HIGH && previous == LOW && millis() - lastTime > debounce) {
    if(enlargerOn == HIGH){
      enlargerOn = LOW; 
    } else {
       enlargerOn = HIGH; 
    }
    lastTime = millis();
  }
  digitalWrite(enlargerLamp, enlargerOn);
  previous == focusState;

  welcomeMessage();


  /*
  String test2;
  String test3;
  int numberlen;
  test2 = String(test1, 2);
  int dotPosition = test2.indexOf('.');
  test2.remove(dotPosition,1);
  numberlen = test2.length();
  for(int i = 0; i<numberlen; i++){
    if(i == dotPosition - 1){
      lc.setChar(0, 7-i, test2[i], true);
      }
    else{
      lc.setChar(0,7-i,test2[i],false);
    }
  }
  delay(50);
  lc.clearDisplay(0);
  */

}

void welcomeMessage() {
  lc.setChar(0,6,'F',false);
  lc.setChar(0,5,'-',false);
  lc.setChar(0,4,'5',false);
  //printT(3, false);
  lc.setChar(0,3,'6',false);
  lc.setLed(0,3,3,false);
  lc.setLed(0,3,1,false);
  lc.setChar(0,2,'o',false);
  lc.setChar(0,1,'p',false);
  delay(debounce);
  lc.clearDisplay(0);
}
/*
void printT(int digit, boolean dp){
  lc.setChar(0,digit,'6',dp);
  lc.setLed(0,digit,3,false);
  lc.setLed(0,digit,1,false);
}
*/
