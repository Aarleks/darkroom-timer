

int timeStart = 999;
int Garbo;
int displayTime;
#include "math.h"
int start = 3000;
int decimals;
int ones;
int tens;
int hundreds;
unsigned long timeRemaining;
int enlargerOn = 0;


int focusLed = 7;
int focusButton = 6;
byte focusState;
byte previousFocus = HIGH;// For determining Focus button change state

boolean clockIsRunning = false;
unsigned long timerDecrement;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(focusLed, OUTPUT);
  pinMode(focusButton, INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  Garbo = fstop2TensSeconds(200);
  focusState = digitalRead(focusButton);
  //Serial.println(focusState);
  if(focusState && focusState != previousFocus){ 
    if(enlargerOn == 1){
      enlargerOn = 0;
    }
    else{
      enlargerOn = 1;
    }
  }
  previousFocus = focusState;
  if(timerDecrement == 0){
    displayTime = Garbo;
  }

  if(enlargerOn == 1){
    if(timeStart == 999){
      timeStart = Garbo;
      timerDecrement = millis();
    }
    else if(millis() - timerDecrement > 99){
      timerDecrement = millis();
      if(timeStart > 0){
        timeStart -= 1;
        displayTime = timeStart;
        }
      else if (timeStart == 0){
        enlargerOn = 0;
        timerDecrement = 0;
        timeStart = 999;
        }
    }
      
  }
  

  digitalWrite(focusLed, enlargerOn);
  decimals = displayTime % 10;
  ones = (displayTime /10) %10;
  tens = displayTime / 100;
  hundreds = displayTime / 1000;
  Serial.print(hundreds);
  Serial.print(tens);
  Serial.print(ones);
  Serial.print('.');
  Serial.println(decimals);
  
}


int fstop2TensSeconds(float FStop){
  float tensValue = 0;
  tensValue = round(10*pow(2,FStop/100));//adapted f-stop to time formula: t=2^(fstop), here time in 1/10th of sec, f-stop in 1/100th
  return tensValue;
}
