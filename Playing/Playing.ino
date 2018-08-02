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

/* we always wait a bit between updates of the display */
unsigned long delaytime=250;
float test1 = 0.00;
//String test2 = String(test1, 2);

/*
 

  
 */

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
}


/*
 This method will display the characters for the
 word "Arduino" one after the other on digit 0. 
 */

void printT(int digit, boolean dp){
  lc.setChar(0,digit,'6',dp);
  lc.setLed(0,digit,3,false);
  lc.setLed(0,digit,1,false);
}

void writeArduinoOn7Segment() {
  lc.setChar(0,6,'a',false);
  lc.setRow(0,5,0x05);
  lc.setChar(0,4,'d',false);
  lc.setRow(0,3,0x1c);
  lc.setRow(0,2,B00010000);
  lc.setRow(0,1,0x15);
  lc.setRow(0,0,0x1D);
  delay(delaytime*6);
  lc.clearDisplay(0);
  delay(delaytime);
} 

/*
  This method will scroll all the hexa-decimal
 numbers and letters on the display. You will need at least
 four 7-Segment digits. otherwise it won't really look that good.
 */
void scrollDigits() {
  //for(int i=0;i<11;i++) {
    /*lc.setChar(0,7,'F',false);
    lc.setChar(0,6,'-',false);
    lc.setChar(0,5,'5',false);
    //lc.setChar(0,4,'6',false);
    printT(4, false);
    lc.setChar(0,3,'o',false);
    lc.setChar(0,2,'p',false);
    //lc.setLed(0,4,3,false);*/
    lc.setColumn(0,5,4);/*
    //lc.setDigit(0,0,19,false);*/
    delay(delaytime*6);
  //}
  lc.clearDisplay(0);
  delay(delaytime);
}



void loop() { 
  writeArduinoOn7Segment();
  scrollDigits();
  delay(delaytime*2);
  lc.clearDisplay(0);
  delay(delaytime);

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
  delay(delaytime*2);
  lc.clearDisplay(0);
}
