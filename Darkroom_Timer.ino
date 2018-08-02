//Monkito is a an open-source darkroom f-stop timer software and hardware, designed by Elia Cottier, in Switzerland.
contact: eliadarkroom@gmail.com


/*LICENSING AND COPYRIGHT:
©Elia Cottier, 2017
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.*/
//sources:
//Display I2C method: modified and adapted from https://learn.sparkfun.com/tutorials/using-the-serial-7-segment-display/example-3-i2c //
//Rotary encoder: modified and adapted from http://www.hobbytronics.co.uk/arduino-tutorial6-rotary-encoder //
//Buttons events management: modified and adapted from http://jmsarduino.blogspot.ch/2009/10/4-way-button-click-double-click-hold.html
//Timer functions: various lost sources//

#include  //Include the Arduino SPI library
#include //necessary for round function

#define relayPin 11 // relay board pin 
#define tonePin 10 // buzzer pin
#define pin_A 2 // pin 2 rotary encoder
#define pin_B 3 // pin 3 rotary encoder
#define focusLedPin 12 // Focus button led pin
#define startLedPin 13 // Start button led pin

////CONSTANTS
//Displays constants
 const byte digitPositions [] = {0x7B,0x7C,0x7D,0x7E};
 const byte displaysDecimals [] = {0b00000000,0b00000001,0b00000010,0b00000100,0b00010000, 0b00100000};//1st to 4th digit decimals places + colon + apostrophe
 const byte brightnessValues[] = {5,10,20,40,80,140,200,250};//7 preset levels for displays brightness 
 const byte displaysAddresses [] = {0x72,0x71};
 const byte Display1 = displaysAddresses[0];
 const byte Display2 = displaysAddresses[1];

//Buttons constants, prime numbers for buttons combination checksums
 const byte buttons [] = {52,50,48,46,44};//buttons pins, 52 timer, 50 strip, 48 prog, 46 focus, 44 start
 const byte numButtons = 5; //number of buttons
 const byte snglClick = 1; //Single clic value for checksum
 const byte dblClick = 19; //Double clic value for checksum
 const byte holdClick = 23; //Long click value for checksum
 const byte longHoldClick = 29; //Long hold click value for checksum

///VARIABLES
//Sketch use
 byte debounce(10);//general debounce delay, ok for buttons and encoder
 byte timerMode;//mode for main loop mode management

//Display variables
 byte brightness;//Brigthness value for setBrightness()
 byte decimal;//Decimal value for setDecimals()

//String variables
 char tempString[5];//Display any value for sprintf, displays are 4 digits, 1 extra char here, just in case...

//Button checker variables
 int DCgap = 300; // max ms between clicks for a double click event
 int holdTime = 500; // ms hold period: how long to wait for press+hold event
 int longHoldTime = 1000; // ms long hold period: how long to wait for press+hold event
 long downTime = -1; // time the button was pressed down
 long upTime = -1; // time the button was released
 
//Buttons status variables
 boolean buttonVal [numButtons] ; // value read from button
 boolean buttonLast [numButtons] ; // buffered value of the button's previous state
 boolean DCwaiting [numButtons] ; // whether we're waiting for a double click (down)
 boolean DConUp [numButtons] ; // whether to register a double click on next release, or whether to wait and click
 boolean singleOK [numButtons] ; // whether it's OK to do a single click
 boolean ignoreUp [numButtons] ; // whether to ignore the button release because the click+hold was triggered
 boolean waitForUp [numButtons] ; // when held, whether to wait for the up event
 boolean holdEventPast [numButtons] ; // whether or not the hold event happened already
 boolean longHoldEventPast [numButtons] ;// whether or not the long hold event happened already
 byte events[numButtons];//buttons events per button
 
////ROTARY ENCODER VARIABLES
 unsigned long currentTime;
 unsigned long loopTime;
 unsigned char encoder_A;
 unsigned char encoder_B;
 unsigned char encoder_A_prev=0;
 volatile unsigned int encoderVal;//must be volatile for interrupt encoder function
 int increment;//size of values increment steps

///TIMER
 byte timerIncrement []= {1,2,5,10,20,25,33,50,100};//Preset 1/100 of f-stop increments for fstopSelector
 byte timerInc;//fstop increment value
 unsigned int FStop;//int value of fStop for timer in 1/100th
 unsigned int tensSeconds;//time in 1/10th of seconds
 unsigned int resumeTime;//Resume time for timer countdown
 double deltaFStop;//f-stop delta for scaling correction


///TEST STRIPS & PROGRAMS
//General variables
 float base;//base f-stop for strip calculations
 byte zones = 11;//number of strip zones, 11 according to the Zone System rationale
 int stepSize;//fstop increment between zones for stripBuilder()
 int roundStep;//100 * stepSize, later rounded for multiples of 3
 
 byte stripID;//strip number for strip matrix data management
 byte stripN;//strip number for single zone retreival
 byte zoneN;//zone number for single zone retreival
 byte exposeMode;//strip exposure mode
 char exposeModeFlag;//expose mode flag for display concatenated info (c,u,_)
 String exposeModeDisp [] = {"_Co_", "_Un_", "_Sn_"};//exposure modes display labels
 
 //BUILDING arrays for all exposure modes
 int testStripStops[11];//test strips of 11 full f-stops
 int testStripSin[11];//test strips times for single mode countdown
 int testStripCover[11];//test strips times for cover mode countdown
 int testStripUncov[11];//test strips times for uncover mode countdown
 
 //STORAGE arrays, strips arrays: index 0 => 2, programs arrays: index 3 => 5
 int seqMatxStops [6][11] = {};//matrix values in f-stops
 int seqMatxSin [6][11] = {};//matrix values in 1/10 of sec.
 int seqMatxCov [6][11] = {};//matrix values in 1/10 of sec.
 int seqMatxUnc [6][11] = {};//matrix values in 1/10 of sec.

///Program variables
 byte progIndex;//Program row index in matrix
 byte progStep;//Program column index in matrix
 byte stepCounter;//Program step counter for step recorder and matrix building

//END OF VARIABLES
 
 
///////////////////////////////////////////SET-UP///////////////////////////////////////////////////////////////////////
void setup()
{
 //Initialize variables
 byte brightness = 40;
 timerInc = timerIncrement[3];//10th of f-stop by default
 deltaFStop = 0;
 exposeMode = 0;//covering mode by default
 exposeModeFlag = 'c';
 stepCounter = 0; 
 
 Serial.begin(9600);
 
 // Set up the LEDs & Relay pins outputs
 pinMode(tonePin, OUTPUT);
 pinMode(relayPin, OUTPUT); 
 pinMode(focusLedPin, OUTPUT); 
 pinMode(startLedPin, OUTPUT);
 
 //initialize display
 Wire.begin();
 clearDisplays();
 setBrightness(brightness);
 
 //welcome message
 send2Display(" Hi ", Display1);
 send2Display("ELIA", Display2);
 
 //initialize relay, beeper, focus and start buttons leds
 digitalWrite(startLedPin, HIGH);
 tone(tonePin, 1760, 100);
 delay(500);
 digitalWrite(relayPin, HIGH);
 digitalWrite(focusLedPin, HIGH); 
 digitalWrite(startLedPin, LOW);
 tone(tonePin, 880, 100);
 
 ///Buttons pins and buttons variables initialization
 for (byte i=0;i<numButtons;i++)
 {
 pinMode(buttons[i], INPUT);
 digitalWrite(buttons[i], HIGH);
 buttonVal[i] = HIGH ;
 buttonLast[i] = HIGH;
 singleOK[i] = true;
 DCwaiting [i] = false;
 DConUp [i] = false;
 ignoreUp [i] = false;
 waitForUp [i] = false;
 holdEventPast [i] = false;
 longHoldEventPast [i]= false;
 events[i] = 0;//all buttons events blanked
 }

///Rotary encoder set up
 pinMode(pin_A, INPUT);
 pinMode(pin_B, INPUT);
 currentTime = millis();
 loopTime = currentTime;
 encoderVal = 0;
 increment = 0;
}
///////////////////////////////////////////END OF SET-UP///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////MAIN LOOP///////////////////////////////////////////////////////////////////////////
void loop()
{
timerModes();//modes and functions management
}
////////////////////////////////////////END OF MAIN LOOP////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////START OF USER INTERFACE MANAGER//////////////////////////////////////////////////////
byte timerModes()
{
 checkButtons();//check buttons events
 
 //Checksum for buttons 1 to 5 combinations. 
 //Uses 1,19,23,29 prime numbers as unambiguous events flags combinations
 
 byte eventCheckSum = events[0] + events[1] + events[2] + events[3] + events[4];//sum of all buttons events at once
 
 switch (eventCheckSum)
 {
 case snglClick://checksum = 1, single click of one single button
 if (events[0] == snglClick) timerMode = 1;//clear encoder and set timerMode to default: f-stop selector
 if (events[1] == snglClick) timerMode = 5;//strip selector
 if (events[2] == snglClick) timerMode = 9;//program selector
 break;
 
 case dblClick://checksum = 19, double click of one single button
 if (events[0] == dblClick) timerMode = 2;//strip zone retreiver
 if (events[1] == dblClick) timerMode = 6;//strip increment selector
 if (events[2] == dblClick) timerMode = 10;//program step selector
 break;
 
 case holdClick://checksum = 23, hold click of one single button
 if (events[0] == holdClick) timerMode = 3;//void
 if (events[1] == holdClick) timerMode = 7;//void
 if (events[2] == holdClick) timerMode = 11;//program builder
 if (events[4] == holdClick) timerMode = 17;//Focus light on/off
 break;
 
 case longHoldClick://checksum = 29, long hold click of one single button
 if (events[0] == longHoldClick) timerMode = 8;//f-stop increment selector
 if (events[1] == longHoldClick) timerMode = 4;//expose mode selector
 if (events[2] == longHoldClick) timerMode = 12;//brightness set-up
 break;
 
 case (2 * snglClick)://checksum = 2, single click of two buttons at once
 if (events[0] == snglClick && events[1] == snglClick) timerMode = 14;//correction selector
 if (events[0] == snglClick && events[2] == snglClick) timerMode = 15;//correction reset
 if (events[1] == snglClick && events[2] == snglClick) timerMode = 16;//timer countdown reset
 break; 
 }

switch (timerMode)
 {
 case 1:
 clearEncoder();
 timerMode = 0;//default mode
 break;
 case 2:
 stripZoneRetreiver();
 break;
 case 3:
 //void
 break;
 case 4:
 exposeModeSelector();
 break;
 case 5:
 stripIDSelector();
 break;
 case 6:
 stripIncrementSelector();
 break;
 case 7:
 //void
 break;
 case 8:
 fStopIncrementSelector();
 break;
 case 9:
 progIndexSelector();
 break;
 case 10:
 progStepSelector();
 break;
 case 11:
 programBuilder();
 break;
 case 12:
 brightnessSelector();
 break;
 case 13:
 //void
 break;
 case 14:
 scaleCalculator();
 break;
 case 15:
 clearCorrection();
 break;
 case 16:
 programCleaner();
 break;
 case 17:
 focusOnOff();
 break;
 default:
 fstopSelector();
 } 
 delay(2*debounce);//avoid flickering
}
///////////////////////////////////////////END OF USER INTERFACE MANAGER///////////////////////////////////


///////////////////////////////////////////START OF CLEARING FUNCTIONS////////////////////////////////////

///Clear modes
byte clearModes()
{
 timerMode = 0;
}

///Clear encoder value
void clearEncoder()
{
 if (encoderVal != 0 ) encoderVal = 0;
}

///Clear scaling correction
void clearCorrection()
{
 deltaFStop = 0;
 clearEncoder();
 
 char corr[4] = {'C','o','r','r'};//local char string variable
 send2Display(corr, Display1);
 
 char off[4] = {'O','f','f',' '};//local char string variable
 send2Display(off, Display2);
 
 delay(1000);//delay for message reading
 timerMode = 1;//back to fstopSelector
}

/////////////////////////////////////END OF CLEARING FUNCTIONS//////////////////////////////////////

////////////////////////////////////START OF SELECTION FUNCTIONS////////////////////////////////////

///Exposure mode selector for test-strips and programs sequences
void exposeModeSelector()
{
 increment = 1;//encoder step increment
 
 setDecimals(0, Display1);
 setDecimals(0, Display2);
 
 attachInterrupt(digitalPinToInterrupt(pin_A), rotaryChecker, CHANGE);
 
 byte m = encoderVal % 3;//read encoder value, modulo 3 division limits values from 0 to 2
 
 switch (m)
 {
 case 0://Cover mode
 exposeMode = 0;
 exposeModeFlag = 'c';//mode flag for display
 send2Display(exposeModeDisp[exposeMode], Display1);//text from exposeModeDisp string array "_Co_"
 Wire.beginTransmission(displaysAddresses[1]);
 
 //cover mode symbol
 Wire.write(0x7B); // send to digit 1
 Wire.write(0b00000000);
 Wire.write(0x7C); // send to digit 2
 Wire.write(0b00001000);
 Wire.write(0x7D); // send to digit 3
 Wire.write(0b01001000);
 Wire.write(0x7E); // send to digit 4
 Wire.write(0b01001001);
 Wire.endTransmission();
 break;


 case 1://Uncover mode
 exposeMode = 1;
 exposeModeFlag = 'u';//mode flag for display
 send2Display(exposeModeDisp[exposeMode], Display1);//text from exposeModeDisp string array "_Un_"
 
 //uncover mode symbol display
 Wire.beginTransmission(displaysAddresses[1]);
 Wire.write(0x7B); // send to digit 1
 Wire.write(0b01001001);
 Wire.write(0x7C); // send to digit 2
 Wire.write(0b01001000);
 Wire.write(0x7D); // send to digit 3
 Wire.write(0b00001000);
 Wire.write(0x7E); // send to digit 4
 Wire.write(0b00000000);
 Wire.endTransmission();
 break;

case 2: //Single full time
 exposeMode = 2;
 exposeModeFlag = '_';//mode flag for display
 send2Display(exposeModeDisp[exposeMode], Display1);//text from exposeModeDisp string array "_Sn_"
 
 //single mode symbol
 send2Display("____", Display2);
 break;
 }
}

///////FSTOP SELECTION FOR SIMPLE TIMER, TEST STRIP BUILDER AND PROGRAM STEP/////////

///F-stop increment selector
void fStopIncrementSelector()
{
 char welcome[4]={'I','n','c','r'};
 send2Display(welcome, Display1);
 setCursorPos(0, Display2);//set cursor position
 setDecimals(0, Display2);//set no decimal
 increment = 1;
 
 attachInterrupt(digitalPinToInterrupt(pin_A), rotaryChecker, CHANGE);
 
 byte i = encoderVal % sizeof(timerIncrement);//Limit values to those available in the presets array
 
 timerInc = timerIncrement[i];//
 
 //Display
 sprintf(tempString, "%4d", timerInc);
 send2Display(tempString, Display2);
 delay(debounce);
}

///F-Stop selector for timer
void fstopSelector()//basic f-stop and time setting function, simple click button 1 
{
 setCursorPos(0, Display2);
 increment = timerInc;//increment as set
 
 ///f-Stop selection
 attachInterrupt(digitalPinToInterrupt(pin_A), rotaryChecker, CHANGE);
 {
 //f-stop lower and upper values limits
 if (encoderVal = 1000) encoderVal = 1000;//f-stop upper limit of 10.00 (= 1024 seconds = +17 min, more than enough !!!)
 
 byte steps = encoderVal / increment;//steps counter from value, for following 1/3rd correction
 
 //f-stop value from encoder ***used by stripBuilder() and timerCountdown*** 
 if (increment == 33) // 1/3rd correction to avoid bad rounding of 3 3rd = 0.99
 {
 int correctedEncoderVal = steps * increment;
 if (steps >= 0 && steps = 2 && steps = 5 && steps = 8 && steps = 11 && steps = 14 && steps = 17 && steps = 20 && steps = 23 && steps = 27 && steps  29) FStop = 1000;
 }
 else
 {
 FStop = encoderVal;
 }
 
 //apply scaling correction
 int corrFStop = FStop + deltaFStop;

//setting displays decimals
 
 //for f-stop
 if (FStop == 0)
 {
 setDecimals(0,Display1);
 }
 else
 {
 setDecimals(2,Display1);
 }
 
 //for time
 if (tensSeconds > 9999)
 {
 setDecimals(0,Display2);
 }
 else
 {
 setDecimals(3,Display2);
 }
 
 //time display
 if (resumeTime != 0)
 {
 tensSeconds = resumeTime;///resume time from timerCountdown() function
 }
 else
 {
 tensSeconds = fstop2TensSeconds(corrFStop);///f-stop conversion to time
 }
 
 //display apostrophe to indicate scaling correction
 if (deltaFStop != 0)
 {
 setDecimals(5,Display2);
 }
 
 ///display f-stop and time
 sprintf(tempString, "%4d", FStop);
 send2Display(tempString, Display1);
 sprintf(tempString, "%4d", tensSeconds);
 send2Display(tempString, Display2);
 
 //timer trigger
 singleButton(3);
 if (events[3] == snglClick )
 {
 timerCountdown(tensSeconds);//timer with f-stop converted in time + enlargment correction time
 }
 }
}

///TEST STRIP NUMBER SELECTOR///
void stripIDSelector()//select strip number for stripBuilder()
{
 increment = 1;
 attachInterrupt(digitalPinToInterrupt(pin_A), rotaryChecker, CHANGE);
 
 stripID = encoderVal % 3;//read encoder value, revolving from 0 to 2

//No decimals to display
 setDecimals(0, Display1);
 setDecimals(0, Display2);
 
 //Character array for display
 char stripName[]={'S','t',stripID + 1,exposeModeFlag};
 send2Display(stripName, Display1);
 
 send2Display("----",Display2);
 delay(debounce);
}

///ZONE INCREMENT SELECTOR AS FRACTIONS///
void stripIncrementSelector()//select interval for test strip, simple click button 2
{
 increment = 1;
 byte stepFraction []= {1,2,3,4,5,6,8,10,12};
 byte fractionDisp;//number constructed to display step as 1:x fraction => 10 + 1-digit fractions, 100 + 2-digits fractions
 
 //select zone step
 attachInterrupt(digitalPinToInterrupt(pin_A), rotaryChecker, CHANGE);
 byte k = encoderVal % sizeof(stepFraction);//read encoder value

stepSize = stepFraction[k];///Variable used by stripBuilder()
 
 //rule to display fractions 10 and 12
 if (stepSize < 10) 
 {
 setCursorPos(3, Display2);//set cursor position
 fractionDisp = 10 + stepSize;//value to display as 1:fraction
 }
 else
 {
 setCursorPos(0, Display2);//set cursor position
 fractionDisp = 100 + stepSize;//value to display as 1:fraction
 }
 
 setDecimals(0, Display1);//set no decimal for display 1
 setDecimals(4, Display2);//set colon for display 2
 
 send2Display("Incr", Display1);//text for display 1
 
 sprintf(tempString, "%4d", fractionDisp);//string for display 2
 send2Display(tempString, Display2);

stripBuilder();//
}

/////////PROGRAM SELECTOR/////////////
void progIndexSelector()//select program number for programBuilder()
{
 increment = 1;
 attachInterrupt(digitalPinToInterrupt(pin_A), rotaryChecker, CHANGE);

byte strip = encoderVal % 3;//read encoder value

switch (strip) //convert index to strip number ***used in stripBuilder***
 {
 case 0:
 progIndex = 3;//Matrix index 
 break;
 case 1:
 progIndex = 4;
 break;
 case 2:
 progIndex = 5;
 break;
 }
 byte progDisp = progIndex - 2;//Program number for display
 
 //Display program specs
 setDecimals(0, Display1);
 setDecimals(0, Display2);
 
 char progN[]={0b01110000,'r',progDisp,exposeModeFlag};//Character array for display
 send2Display(progN, Display1);
 send2Display("----",Display2);

//Start button check to run the sequence
 singleButton(3);
 if (events[3] == snglClick)//check start button
 {
 switch (exposeMode)//exposure mode check to display mode before calling for timer countdown
 {
 case 1:
 matrixCountdown(stepCounter, progIndex, seqMatxUnc[progIndex]);//uncovering mode
 break;
 case 2:
 matrixCountdown(stepCounter, progIndex, seqMatxSin[progIndex]);//Single mode
 break;
 default:
 matrixCountdown(stepCounter, progIndex, seqMatxCov[progIndex]);//default cover mode
 }
 }
}

//////PROGRAM STEP SELECTOR
void progStepSelector()//select strip for strip builder
{
 increment = 1;
 attachInterrupt(digitalPinToInterrupt(pin_A), rotaryChecker, CHANGE);
 byte progDisp = progIndex - 2;
 progStep = encoderVal % zones;//read 11 encoder values from 0 to 10
 byte st = progStep + 1;//progStep for display
 
 //Display program and step display 1
 setDecimals(4, Display1);//Colon on
 setDecimals(2, Display2);
 
 if (st  9 => 1 and array (index + 1)-10
 send2Display(stepDisp, Display1);
 }
 
 int stVal = seqMatxStops[progIndex][progStep];//Display f-stop value on display 2
 if (stVal == 0) 
 {
 send2Display(" ", Display2);
 }
 else 
 {
 sprintf(tempString, "%4d", stVal);
 send2Display(tempString, Display2);
 } 
 delay(100); 
}

////PROGRAM BUILDER, intended to write programs values into the matrices
void programBuilder()
{
 int progStepVal = FStop;//F-stop value from f-stop selector
 
 //Display labels
 byte progLabel = progIndex - 2;//progIndex for display, index 3 => prog 1
 byte progStepLabel = progStep + 1;
 
 //Counters
 stepCounter = progStep + 1;
 
 //first program step values
 seqMatxStops[progIndex][progStep] = progStepVal;//fstops array
 seqMatxSin[progIndex][progStep] = fstop2TensSeconds(progStepVal);//single time array
 seqMatxCov[progIndex][0] = seqMatxSin[progIndex][0];//
 
 programMatrixBuilder(progIndex, progStep);//matrix building from first step values
 
 //Display Program data
 setDecimals(4, Display1);//colon on display 1
 
 //Display1: exposure mode, program number, program step
 if (progStepLabel >9 )
 {
 char progT[]={0b01110000,progLabel,1,progStepLabel-10};//Character array for display.P + program number + program step
 send2Display(progT, Display1);
 }
 else
 {
 char progT[]={0b01110000,progLabel,' ',progStepLabel};//Character array for display.P + program number + program step
 send2Display(progT, Display1);
 }
 
 //Display 2: 
 setDecimals(2, Display2);
 sprintf(tempString, "%4d", seqMatxStops[progIndex][progStep]);//f-stop display
 send2Display(tempString, Display2);
 
 delay(500);//slow down to enable reading
 setCursorPos(0, Display1);
 timerMode = 1;//back to timer mode
}

///Matrix builder used by programBuilder()
void programMatrixBuilder(byte pindex, byte sindex)//Cover and uncover program matrix builder
{
 //cover mode loop
 for (byte j = 1; j < sindex + 1; j++)
 {
 //time of zone (n) = full time of zone (n) - time of zone (n-1)
 seqMatxCov[pindex][sindex] = seqMatxSin[pindex][sindex] - seqMatxSin[pindex][sindex - 1] ;
 }
 
 //uncover mode loop, reverse sequence of cover mode
 for (byte i = 0 ; i < sindex + 1; i++)
 {
 //time of zone (n) = cover mode time of zone (n-1)
 seqMatxUnc[pindex][i] = seqMatxCov[pindex][sindex - i];
 }
}

void programCleaner()//clean selected program
{
 byte j;
 byte progNum = progIndex - 2;

for (j = 0; j < zones ; j++)
 {
 seqMatxStops[progIndex][j] = 0;
 seqMatxSin[progIndex][j] = 0;
 seqMatxCov[progIndex][j] = 0;
 seqMatxUnc[progIndex][j] = 0;
 }
 char eraseD[]={'C','l','r',' '};
 send2Display(eraseD,Display1);
 char eraseD2[]={0b01110000,' ',progNum,' '};
 send2Display(eraseD2,Display2);
 delay(1000);
 timerMode = 1;
}
/////////END OF PROGRAM FUNCTIONS//////////////////////

////////SCALE CALCULATOR//////////////////////////////
void scaleCalculator()//determine f-stop scaling factor from length change proportion, based on 10cm
{
 byte incr = timerInc;//saves timer increment setting
 timerInc = timerIncrement [0];//set timer increment to 1 for highest time resolution
 setDecimals(0,Display1);
 setDecimals(2,Display2);
 send2Display("Corr",Display1);
 
 double lengthRatio = 0;
 lengthRatio = (tensSeconds);//int tensSeconds to double for following math functions
 deltaFStop = 100*(log10(pow(lengthRatio/100,2))/log10(2));
 int deltaFStopD = deltaFStop;//int value for display
 
 sprintf(tempString, "%4d", deltaFStopD);
 send2Display(tempString, Display2);
 delay(2000);//delay for human reading
 clearEncoder();
 timerInc = incr;//back to previous increment seeting
 timerMode = 1;
}
////////END OF SCALE CALCULATOR////////////////////////

////////TEST STRIPS BUILDER FUNCTIONS ////////////////
void stripBuilder()//build the test strip array with zones full times ***used by stripIncrementSelector()***
{
 byte i;//strip matrix row counter (3 max)
 //byte j;//strip matrix column counter (11 max)
 
 int stripBase = FStop; //strip base is FStop from fstopSelector()
 int stripFStop;//The f-stop of a specific zone
 
 setCursorPos(0, Display2);//set cursor position for display, 0 = right-most

//Set arrays first values 
 testStripStops[0] = stripBase;//fstop
 testStripSin[0] = fstop2TensSeconds(testStripStops[0]);//time in 1/10 s.
 testStripCover[0] = testStripSin[0];//time for cover exposure mode

//loop for the remaining values from index 1 to index 10
 for (i = 1; i < zones; i++)//zones = constant for strip size 
 {
 roundStep = 100/stepSize;//transform fraction in full values
 if ((stepSize == 3) && ((i+1) % 3 == 0))//condition to identify multiples of 3/3
 {
 roundStep = roundStep + 1;//rule to reach 3/3 = 1, 100 vs 99
 }
 else
 {
 roundStep = roundStep;
 }
 testStripBuilder(zones, i, roundStep);//Build full time strip
 }
 matrixBuilder(zones, stripID);//Build matrix with cover, uncover and single full times
 
 singleButton(3);//check for timerCountdown
 if (events[3] == snglClick)//check start button
 {
 switch (exposeMode)//check exposure mode
 {
 case 1:
 matrixCountdown(zones, stripID, seqMatxUnc[stripID]);//uncovering mode matrix
 break;
 case 2:
 matrixCountdown(zones, stripID, seqMatxSin[stripID]);//Single mode matrix
 break;
 default:
 matrixCountdown(zones, stripID, seqMatxCov[stripID]);//default cover mode matrix
 }
 }
 delay(debounce);
}

void testStripBuilder(byte zones, byte index, int rStep)//used by stripBuilder()
{
 byte k = (zones-1) - index;//reverse index sequence
 testStripStops [index] = testStripStops [index-1] + rStep;//variable, add fstop step size to previous fstop//
 testStripSin [index] = fstop2TensSeconds(testStripStops [index]);//time corresponding to fStop
 testStripCover [index] = testStripSin [index] - testStripSin [index-1];//time in cover mode, start with the shortest time
 testStripUncov[k] = testStripCover[index];//uncover sequence = cover sequence in reverse (k = reverse of index)
}

void matrixBuilder(byte zonesNb, byte stripId)//used by stripBuilder()
{
 for (byte j = 0 ; j  from 0 to 32
 stripN = encoderVal % 3;//read encoder value
 if (encoderVal 10 && encoderVal  21 )//22 to 32, 3rd strip, 11 zones
 {
 stripN = 2;
 zoneN = encoderVal - (stripN * zones);
 }
 
 if (encoderVal > 31 )//upper limit, last zone
 {
 encoderVal = 32;
 stripN = 2;
 zoneN = 10;
 }

ten = 10 * stripN + 10;
 
 //display selected strip
 byte zone = zoneN + 1;
 
 if (zone = 0;x--)
 {
 digitalWrite(relayPin, HIGH);//enlarger light ON
 digitalWrite(startLedPin, HIGH);//Start button led ON
 timeCounter--; // Decrease of the counter

//time display on display 2
 sprintf(tempString, "%4d", timeCounter);
 send2Display(tempString, Display2);
 
 //start button resume function
 if (digitalRead(buttons[3]) == LOW )
 {
 resumeTime = x;//set resume time
 x=0;//end of timer loop
 } 
 
 //bip conditions
 if (x!= 0 && x % 10 == 0) //bip every full second or 10 1/10th
 {
 tone(10,440,10);
 } 
 else if (x == 0)//final bip
 {
 tone(10,1780,20);
 }
 
 //10Hz adjusted refresh rate to reach < 0.5 % precision 
 delay(99); 
 }
 //Time elapsed, lights off
 digitalWrite(relayPin, LOW);//enlarger light OFF
 digitalWrite(startLedPin, LOW);//Start button led OFF
}

void matrixCountdown(byte steps, byte rowNr, int strip[])//loop to countdown strips and program sequences
{ 
 for (byte i = 0; i < steps; i++)//loop inside strip array
 {
 //Display 1: f-stop
 sprintf(tempString, "%4d", seqMatxStops[rowNr][i]);//display 1st fstop
 send2Display(tempString, Display1);
 setDecimals(2, Display1);
 
 //Display 2: next zone number
 setCursorPos(4, Display2);
 sprintf(tempString, "%4d", i+1);//display zone number, zone index + 1
 setDecimals(0,Display2);
 send2Display(tempString, Display2);
 
 //start countdown
 singleButton(3);
 if (events[3] == holdClick) 
 {
 timerMode = 1;//abort sequence without affecting values
 break;
 }
 if (events[3] == snglClick)
 {
 timerCountdown(strip[i]);//zone countdown
 events[3] = 0;//reset start button status
 }
 else i--;//i = i - 1;
 }
 clearDisplays();
 send2Display("End ",Display1);
 delay(1000);//time to display "End"
}

//////////////////////////////////////////END OF TIMER FUNCTIONS////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////DISPLAY, BUTTONS, ENCODER AND OTHER FUNCTIONS////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Focus light on and off
void focusOnOff()
{
 digitalWrite(relayPin, !digitalRead(relayPin));
 digitalWrite(focusLedPin, !digitalRead(focusLedPin));
 timerMode = 0;
}


//////////////////////////////////////////START OF DISPLAY FUNCTIONS////////////////////////////////////////

///Functions for BOTH displays
//brightness intensity selection
void brightnessSelector()//timerMode 6, 
{
 increment = 1;//encoder increment size
 
 attachInterrupt(digitalPinToInterrupt(pin_A), rotaryChecker, CHANGE);
 byte n = encoderVal % 8;//limited to available values number
 setBrightness(brightnessValues[n]);
 
 //display function name
 setDecimals(0, Display2);
 send2Display("brit", Display1);
 
 //display brightness value
 sprintf(tempString, "%4d", brightnessValues[n]);
 send2Display(tempString, Display2);
 
 delay(2*debounce);
}

void setBrightness(byte brightness)//set brightness
{
 for (int i = 0; i<2 ; i++)
 {
 Wire.beginTransmission(displaysAddresses[i]);
 Wire.write(0x7A); // Set brightness command byte
 Wire.write(brightness); // brightness data byte
 Wire.endTransmission();
 }
}

void clearDisplays()//clear both displays
{
 for (byte i=0; i<2 ; i++)
 {
 byte disp = displaysAddresses[i];
 Wire.beginTransmission(disp);
 Wire.write(0x76); // Clear display command
 Wire.endTransmission();
 }
}

///functions for ONE single display

void setDecimals(byte decimal, int disp)//set decimal index from right to left (0 to 3), colon = 4, apostrophe = 5
{
 Wire.beginTransmission(disp);
 Wire.write(0x77);//decimal command byte
 Wire.write(displaysDecimals[decimal]);//decimal position
 Wire.endTransmission();
}

void setCursorPos(byte cursr, int disp)//set cursor position for display
{
 byte cursPositions [] = {0x00,0x01,0x02,0x03};//0 to 3 position, from left to right
 Wire.beginTransmission(disp);
 Wire.write(0x79);//cursor command byte
 Wire.write(cursPositions[cursr]);//position number
 Wire.endTransmission();
}

void send2Display(String ready2Print, int disp)//display the first 4 characters of a string
{
 Wire.beginTransmission(disp);
 for (int i=0; i<4; i++)
 {
 Wire.write(ready2Print[i]);
 }
 Wire.endTransmission();
}

//////////////////////////////////////////END OF DISPLAY FUNCTIONS////////////////////////////////////////////


//////////////////////////////////////////START OF BUTTONS FUNCTIONS////////////////////////////////////////////

void checkButtons()//loop to check all buttons with singleButton()
{
 for (byte i=0; i  debounce) 
 {
 downTime = millis();
 ignoreUp[button] = false;
 waitForUp[button] = false;
 singleOK[button] = true;
 holdEventPast[button] = false;
 longHoldEventPast[button] = false;
 if ((millis()-upTime)  debounce) 
 { 
 if (not ignoreUp[button]) 
 {
 upTime = millis();
 if (DConUp[button] == false) DCwaiting[button] = true;
 else 
 {
 events[button] = dblClick;
 DConUp[button] = false;
 DCwaiting[button] = false;
 singleOK[button] = false;
 }
 }
 }
 // Test for normal click event: DCgap expired
 if ( buttonVal[button] == HIGH && (millis()-upTime) >= DCgap && DCwaiting[button] == true && DConUp[button] == false && singleOK[button] == true) 
 {
 events[button] = snglClick;
 DCwaiting[button] = false;
 }
 
 // Test for hold
 if (buttonVal[button] == LOW && (millis() - downTime) >= holdTime) 
 {
 // Trigger "normal" hold
 if (not holdEventPast[button]) 
 {
 events[button] = holdClick;
 waitForUp[button] = true;
 ignoreUp[button] = true;
 DConUp[button] = false;
 DCwaiting[button] = false;
 downTime = millis();
 holdEventPast[button] = true;
 }
 //Trigger "long" hold
 if ((millis() - downTime) >= longHoldTime) 
 {
 if (not longHoldEventPast[button]) 
 {
 events[button] = longHoldClick;
 longHoldEventPast[button] = true;
 }
 }
 }
 buttonLast[button] = buttonVal[button];
}
//////////////////////////////////////////END OF BUTTONS FUNCTIONS////////////////////////

//////////////////////////////////////////START OF ROTARY ENCODER FUNCTIONS///////////////

void rotaryChecker()
{
 int i = increment;//increment value from each function using this function
 currentTime = millis();// get the current elapsed time
 if(currentTime >= (loopTime + 5))// 5ms since last check of encoder = 200Hz 
 {
 encoder_A = digitalRead(pin_A); // Read encoder pins
 encoder_B = digitalRead(pin_B); 
 if((!encoder_A) && (encoder_A_prev))// A has gone from high to low 
 {
 if(encoder_B) 
 {
 // B is high so clockwise
 tone(tonePin, 1760, 10);
 encoderVal += i;
 }
 else
 {// B is low so counter-clockwise
 tone(tonePin, 880, 10);
 if (encoderVal <= 0)//avoid negative values
 encoderVal = 0;
 else
 {
 encoderVal -= i;
 }
 } 
 } 
 encoder_A_prev = encoder_A; // Store value of A for next time
 loopTime = currentTime; // Updates loopTime
 } 
 delay(2 * debounce); 
}
//////////////////////////////////////////END OF ROTARY ENCODER FUNCTIONS////////////////////////////////////////////
