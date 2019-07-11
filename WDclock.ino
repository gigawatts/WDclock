/* Firmware for WD Clock
 * Board: Arduino Pro Mini, ATmega328 (3.3v, Internal 8 MHz)
 * 6 x 1.25" 7-segment Red LED displays driven by a MAX7219
 * HC-05 bluetooth module connected to serial port
 * RTC DS1307 with a DS32KHZ crystal
 
 * Pins
 ## MAX7219
 pin D8 is connected to the DataIn 
 pin D7 is connected to the CLK 
 pin D6 is connected to LOAD 
 
 ## RTC DS1307
 pin A4 is connected to SDA (RTC 5) [white]
 pin A5 is connected to SCL (RTC 6) [green]
 
 ## Button
 GND --- Button --- D2
 
 ## LDR
 pin A0 and GND are connected to LDR
 */

#include <LedControl.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>  // https://github.com/PaulStoffregen/DS1307RTC a basic DS1307 library that returns time as a time_t
#include <Timezone.h>  // https://github.com/JChristensen/Timezone
#include <Bounce2.h>

 
#define BUTTON_PIN 2

// Instantiate a Bounce object
Bounce debouncer = Bounce();

LedControl lc=LedControl(8,7,6,1);

const byte pot_pin = 0;         // Photo-resistor pin
double k = 0.01;                  // k value, was 0.04
double average = 800;                // the average
int i = 0;

int update = 0;
unsigned long updatemillis = 0;

unsigned long knockmillis = 0;

const int threshold = 20;
int mode = 1;

// US Central Time Zone (Chicago)
TimeChangeRule usCDT = {"CDT", Second, Sun, Mar, 2, -300};    //Daylight time = UTC - 5 hours
TimeChangeRule usCST = {"CST", First, Sun, Nov, 2, -360};     //Standard time = UTC - 6 hours
Timezone usCT(usCDT, usCST);

// If TimeChangeRules are already stored in EEPROM, comment out the three lines above and uncomment the line below.
//Timezone usCT(100);       //assumes rules stored at EEPROM address 100


TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

unsigned long previousMillis = 0;
const long interval = 10000; 

void setup() {
  Serial.begin(9600);
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) {
     Serial.println("Unable to sync with the RTC");
  } else {
     Serial.print("RTC has set the system time: ");
     time_t utc = now();
     printDateTime(utc, "UTC");
  }
  
  lc.shutdown(0,false);
  lc.setIntensity(0,8);  // set the brightness
  lc.clearDisplay(0);    // and clear the display
  
  // Button and debounce setup
  // Setup the button with an internal pull-up :
  pinMode(BUTTON_PIN,INPUT_PULLUP);

  // After setting up the button, setup the Bounce instance
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(10); // interval in ms  
}


void loop () {
  if(Serial.available())
  {
     time_t t = processSyncMessage();
     if(t >0)
     {
        RTC.set(t);   // set the RTC and the system time to the received value
        setTime(t);
        updatemillis = millis();
        
        Serial.print("Time set to: ");
        time_t utc = now();
        printDateTime(utc, "UTC");
     }
  }
  

  if( millis() - updatemillis < 10000 )
  {  update = 1; }
  else
  {  update = 0; }

    time_t utc = now();
    time_t local = usCT.toLocal(utc, &tcr);

    // Print the current UTC and local time to serial every 30 seconds
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;
      Serial.println();
      printDateTime(utc, "UTC");
      printDateTimeLocal(local, tcr -> abbrev, tcr -> offset);
    }

    // Split each variable up into 100/10/1 variables
    int yearTho = year(local) / 1000;
    int yearHun = (year(local) / 100) % 10;
    int yearTen = (year(local) % 100) / 10;
    int yearOne = year(local) % 10;
    
    int monthTen = month(local) / 10;
    int monthOne = month(local) % 10;
    
    int dateTen = day(local) / 10;
    int dateOne = day(local) % 10;
    
    int hourTen = hour(local) / 10;
    int hourOne = hour(local) % 10;

    int hour12Ten = hourFormat12(local) / 10;
    int hour12One = hourFormat12(local) % 10;
    
    int minTen = minute(local) / 10;
    int minOne = minute(local) % 10;
    
    int secTen = second(local) / 10;
    int secOne = second(local) % 10;
    int dotMod = secOne % 2;
    
  // Determine lighting and change display brightness
  int cdsraw = analogRead(pot_pin);
  average = average + (cdsraw - average) * k;
  int brightness = map(average, 0, 1015 , 15, 0);         // Map
  lc.setIntensity( 0, brightness);                       // Set the brightness


// ############################## MODES  ####################################
  
  // Update the Bounce instance :
  debouncer.update();

  // Call code if Bounce fell (transition from HIGH to LOW) :
  if ( debouncer.fell() ) {
      mode += 1;
      knockmillis = millis();
  }
  
  if (mode > 7) {
    mode = 1;
  }
  
/* ************************************************************** */
  
  if (mode == 1) {
    // Display hours (24h) and minutes, centered, with flashing center dot
    lc.setChar(0,5,' ',false);
    lc.setDigit(0,4,hourTen,false);
    lc.setDigit(0,3,hourOne,dotMod);
    lc.setDigit(0,2,minTen,false);
    lc.setDigit(0,1,minOne,false);
    lc.setChar(0,0,' ',update);

    // Hours, Minutes, and Seconds, no flashing dots
//    lc.setDigit(0,5,hourTen,false);
//    lc.setDigit(0,4,hourOne,true);
//    lc.setDigit(0,3,minTen,false);
//    lc.setDigit(0,2,minOne,true);
//    lc.setDigit(0,1,secTen,false);
//    lc.setDigit(0,0,secOne,false);
   }

/* ************************************************************** */
  if (mode == 2) {

    // Hours (12h), Seconds, and AM or PM
    lc.setDigit(0,5,hour12Ten,false);
    lc.setDigit(0,4,hour12One,dotMod);
    lc.setDigit(0,3,minTen,false);
    lc.setDigit(0,2,minOne,false);
    lc.setChar(0,1,' ',false);
    if (isAM(local)) { 
      lc.setChar(0,0,'A',update);
    } else {
      lc.setChar(0,0,'P',update);
    }
  }

/* ************************************************************** */
  if (mode == 3) {  // Display month, day, year, alternating flashing dots between 2nd and 4th digits
    lc.setDigit(0,5,monthTen,false);
    lc.setDigit(0,4,monthOne,dotMod);
    lc.setDigit(0,3,dateTen,false);
    lc.setDigit(0,2,dateOne,!dotMod);
    lc.setDigit(0,1,yearTen,false);
    lc.setDigit(0,0,yearOne,update);
  }

/* ************************************************************** */
  if (mode == 4) 
  {  // Timezone and offset
    String tzName = tcr -> abbrev;
    int tzOffset = (tcr -> offset) / 60;
    char tzPosNeg;
    if (tzOffset > 0)
    {
      tzPosNeg = ' ';
    } else {
      tzPosNeg = '-';
    }
    
    lc.setDigit(0,5,tzName.charAt(0),false);
    lc.setDigit(0,4,tzName.charAt(1),false);
    lc.setDigit(0,3,tzName.charAt(2),false);
    lc.setChar(0,2,' ',false);
    lc.setDigit(0,1,tzPosNeg,false);
    lc.setDigit(0,0,tzOffset,false);
  }
  
/* ************************************************************** */
  if (mode == 5) {     // Brightness Debug Display
    int avg = int(average);
    int rawTho = avg / 1000;
    int rawHun = (avg / 100) % 10;
    int rawTen = (avg % 100) / 10;
    int rawOne = avg % 10;
    int brTen = brightness / 10;
    int brOne = brightness % 10;
  
    lc.setDigit(0,5,rawTho,false);
    lc.setDigit(0,4,rawHun,false);
    lc.setDigit(0,3,rawTen,false);
    lc.setDigit(0,2,rawOne,true);
    lc.setDigit(0,1,brTen,false);
    lc.setDigit(0,0,brOne,false);
  }  
   
  
/* ************************************************************** */
  if (mode == 6) {  // Display countdown to event
    knockmillis = millis(); // Stay in this mode
    unsigned long timerDate = 1577858400; // 1/1/2020 06:00:00 GMT  
    time_t t = now();
    //unsigned long secsLeft = timerDate - t;
    
    long timerDays=0;
    long timerHours=0;
    long timerMins=0;
    long timerSecs=0;
    timerSecs = timerDate - t; //convect milliseconds to seconds
    timerMins=timerSecs/60; //convert seconds to minutes
    timerHours=timerMins/60; //convert minutes to hours
    timerDays=timerHours/24; //convert hours to days
    timerSecs=timerSecs-(timerMins*60); //subtract the coverted seconds to minutes in order to display 59 secs max 
    timerMins=timerMins-(timerHours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
    timerHours=timerHours-(timerDays*24); //subtract the coverted hours to days in order to display 23 hours max
    
    int timerDayHun = (timerDays / 100) % 10;
    int timerDayTen = (timerDays % 100) / 10;
    int timerDayOne = (timerDays % 10);
    
    int timerHourTen = timerHours / 10;
    int timerHourOne = timerHours % 10;
    int timerMinuteTen = timerMins / 10;
    int timerMinuteOne = timerMins % 10;
    int timerSecondTen = timerSecs / 10;
    int timerSecondOne = timerSecs % 10;
    
    //Display results
    if (timerDate < t) { // Display after event occurs
      if(dotMod) {
        //" 2020 "
        lc.setChar(0,5,' ',false);
        lc.setDigit(0,4,2,false);
        lc.setDigit(0,3,0,false);
        lc.setDigit(0,2,2,false);
        lc.setDigit(0,1,0,false);
        lc.setChar(0,0,' ',false);
        
        //" B-DAY"
//        lc.setChar(0,5,' ',false);
//        lc.setChar(0,4,'B',false);
//        lc.setChar(0,3,'-',false);
//        lc.setChar(0,2,'D',false);
//        lc.setChar(0,1,'A',false);
//        lc.setRow(0,0,B00111011); // Y

      }
      else {
        // " HAPPY"
        lc.setChar(0,5,' ',false);
        lc.setChar(0,4,'H',false);
        lc.setChar(0,3,'A',false);
        lc.setChar(0,2,'P',false);
        lc.setChar(0,1,'P',false);
        lc.setRow(0,0,B00111011); // Y
      }
     }
    else { // Display before event occurs
      if (timerDays>0) // days will displayed only if value is greater than zero (more than 24 hours before event)
        {
          Serial.print("timerDays = "); Serial.print(timerDays);
          Serial.print("   timerHours = "); Serial.println(timerHours);
          lc.setDigit(0,5,timerDayHun,false);
          lc.setDigit(0,4,timerDayTen,false);
          lc.setDigit(0,3,timerDayOne,false);
          //lc.setDigit(0,2,' ',false);
          lc.setRow(0,2,B00000000);
          lc.setDigit(0,1,timerHourTen,false);
          lc.setDigit(0,0,timerHourOne,dotMod);
        }
      else //less than 24 hours before event
        {
          lc.setDigit(0,5,timerHourTen,false);
          lc.setDigit(0,4,timerHourOne,true);
          lc.setDigit(0,3,timerMinuteTen,false);
          lc.setDigit(0,2,timerMinuteOne,true);
          lc.setDigit(0,1,timerSecondTen,false);
          lc.setDigit(0,0,timerSecondOne,update);
        }
    }
  }

/* ************************************************************** */

  if (mode == 7) { // Display test, light up all segments and dots
    lc.setRow(0,5,B11111111);
    lc.setRow(0,4,B11111111);
    lc.setRow(0,3,B11111111);
    lc.setRow(0,2,B11111111);
    lc.setRow(0,1,B11111111);
    lc.setRow(0,0,B11111111);
  }
      
    delay(20);
}  // End of Main Loop


// ------------------------------------------------------------------
/* Functions -------------------------------------------------------- */

// format and print a time_t value, with a time zone appended.
void printDateTime(time_t t, const char *tz)
{
    char buf[32];
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
        hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
    Serial.println(buf);
}

void printDateTimeLocal(time_t t, const char *tz, int offsetM)
{
    int offset = offsetM / 60;
    char buf[32];
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s %d",
        hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz, offset);
    Serial.println(buf);
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*  code to process time sync messages from the serial port   */
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by unix time_t as ten ascii digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message

time_t processSyncMessage() {
  // return the time if a valid sync message is received on the serial port.
  while(Serial.available() >=  TIME_MSG_LEN ){  // time message consists of a header and ten ascii digits
    char c = Serial.read() ; 
    //Serial.print(c);  
    if( c == TIME_HEADER ) {       
      time_t pctime = 0;
      for(int i=0; i < TIME_MSG_LEN -1; i++){   
        c = Serial.read();          
        if( c >= '0' && c <= '9'){   
          pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
        }
      }
     return pctime; 
    }
  }
  return 0;
}
