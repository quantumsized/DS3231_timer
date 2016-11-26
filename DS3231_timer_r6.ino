/****
This revision of the DS3231 Timer is a basic setup for driving a device on a timer.
In the loop function, it test to see what time of year and automatically sets the DS3231 to PDT or PST.
Second Sunday of March set forward and first Sunday in November set back by one hour.
****/
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>

DS3231  rtc(SDA, SCL);

//define RTC address
#define DS3231_I2C_ADDRESS 0x68 // Set address for RTC so as there is no conflict
LiquidCrystal_I2C lcd(0x27, 16, 2);
String tod = "AM";
int ledPin = 13;         // Set our LED pin. This is the driver pin for control any further device too
#define LCD_Backlight 10 // Define backlight pin for LCD
boolean showSeconds = true; // Set to true if you want to show seconds
boolean showBlink = true;  // Set to true if you want to show the colin between blink
boolean useDST = true;      // Set to true for use of DST. Not auto-set as to geolocation.

//variables for ui
boolean mblinkOn = true; //visibility of ':' between hour and minutes
boolean sblinkOn = true; //visibility of ':' between minutes and seconds
boolean setVisible = false; //visibility of the set time ui

// These are set as 24 hour clock
byte startHour = 21;    // Set our start and end times
byte startMinute = 0;  // Don't add leading zero to hour or minute - 7, not 07
byte endHour = 8;      // Use 24h format for the hour, without leading zero
byte endMinute = 30;   // Set end minute. Again, don't add leading zero to hour or minute - 7, not 07

byte validStart = 0;    // Declare and set to 0 our start flag
byte poweredOn = 0;     // Declare and set to 0 our current power flag
byte validEnd = 0;      // Declare and set to 0 our end flag

unsigned long previousMillis = 0; // Var for timer governing cycle tests
const long interval = 1000; // Cycle time for running tests set in milliseconds
int DST = 0; // Used for setting Daylight Savings Time for RTC

int hourVal = 0;
int minuteVal = 0;
int secondVal = 0;
int monthVal = 0;
int dayVal = 0;
int yearVal = 0;

//convert normal decimal numbers to binary coded decimals
byte decToBcd(byte val) {
  return ( (val / 10 * 16) + (val % 10) );
}
//convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val) {
  return ( (val / 16 * 10) + (val % 16) );
}

void setup () {
  lcd.init();
  lcd.backlight();
  pinMode(ledPin, OUTPUT);   // Set our LED as an output pin
  digitalWrite(ledPin, LOW); // Set the LED to LOW (off)
  Wire.begin();              // Start our wire and real time clock
  rtc.begin();
  pinMode(LCD_Backlight, OUTPUT);
  analogWrite(LCD_Backlight, 64);
  clearLCD();
  /*Use below code to set time and date once from code
  it will run at every reset and you will lose the time on the RTC
  format is (sec, min, hr, day of week, day of month, month, year)
  dayOfWeek is 1-7 and 1 being Monday 7 being Sunday. Year is last two eg. 16 for 2016 */
  //setRTCTime(secondVal, minuteVal, hourVal, dayOFweek, dayVal, monthVal, yearVal);
}

//easy and dirty way to clear the LCD
void clearLCD () {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 0);
}

// Read the time and date from the RTC
void readRTCTime(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year) {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

//set the time and date to the RTC
void setRTCTime(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year) {
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

//reads the RTC time and prints it to the top of the LCD
void printTime() {
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  
  //retrieve time
  readRTCTime(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  
  //print to lcd top
  lcd.setCursor(0,0);
  lcd.print("Time:");
  // Hour:Minute:Second
  // to display 12 hour time ...
  if(hour<10) {lcd.print(" ");}
  if(hour-12<10 && hour-12>0) {lcd.print(" ");}
  if(hour>12){
    lcd.print(hour-12,DEC);
    tod = "PM";
  }else{
    lcd.print(hour, DEC);
    tod = "AM";
  }
  if(showBlink == true) {
    if (mblinkOn == true) {
      lcd.print(" ");
      mblinkOn = false;
    } else if (mblinkOn == false) {
      lcd.print(":");
      mblinkOn = true;
    }
  }else{
    lcd.print(":");
  }
  if (minute<10) { lcd.print("0"); }
  lcd.print(minute, DEC);
  if(showSeconds == true) {
    if(showBlink == true) {
      if (sblinkOn == true) {
        lcd.print(" ");
        sblinkOn = false;
      } else if (sblinkOn == false) {
        lcd.print(":");
        sblinkOn = true;
      }
    }else{
      lcd.print(":");
    }
    if (second<10) { lcd.print("0"); }
    lcd.print(second, DEC);
  }
  lcd.print(" ");
  lcd.print(tod);
  delay(10);
}

void loop () {
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { // use millis() timer rather than delay()
    previousMillis = currentMillis; // update var for checking current cycle in timer
    //retrieve time
    readRTCTime(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
    /**  Start DST setting block
         You will have to check when DST changes in your area and set this manually.  **/
    if(useDST == true) {
      if (dayOfWeek == 7 && month == 3 && dayOfMonth >= 7 && dayOfMonth <= 14 && hour == 2 && DST == 0) {
        // DS3231 seconds, minutes, hours, day, date, month, year
        setRTCTime(second, minute, 3, dayOfWeek, dayOfMonth, month, year);
        DST = 1;
      } else if (dayOfWeek == 7 && month == 11 && dayOfMonth >= 1 && dayOfMonth <= 7 && hour == 2 && DST == 1) {
        // DS3231 seconds, minutes, hours, day, date, month, year
        setRTCTime(second, minute, 1, dayOfWeek, dayOfMonth, month, year);
        DST = 0;
      }
    }

    if (second == 0) { // Only process if we have ticked over to new minute
      if (poweredOn == 0) {  // Check if lights currently powered on
        checkStartTime();    // If they are not, check if it's time to turn them on
      } else {
        checkEndTime();      // Otherwise, check if it's time to turn them off
      }

      if (validStart == 1) { // If our timer is flagged to start, turn the lights on
        turnLightsOn();
      }
      if (validEnd == 1) {   // If our time is flagged to end, turn the lights off
        turnLightsOff();
      }
    }
    printTime();
  }
}

byte checkStartTime() {
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  //retrieve time
  readRTCTime(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  if (hour == startHour && minute == startMinute) {
    validStart = 1;  // If our start hour and minute match the current time, set 'on' flags
    poweredOn = 1;
  } else {
    validStart = 0;  // Otherwise we don't need to power up the lights yet
  }

  return validStart; // Return the status for powering up
}

byte checkEndTime() {
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  //retrieve time
  readRTCTime(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  if (hour == endHour && minute == endMinute) {
    validEnd = 1;    // If our end hour and minute match the current time, set the 'off' flags
    poweredOn = 0;
  } else {
    validEnd = 0;    // Otherwise we don't need to power off the lights yet
  }

  return validEnd;   // Return the status for powering off
}

void turnLightsOn() {
  digitalWrite(ledPin, HIGH);  // Turn on the LED
}

void turnLightsOff() {
  digitalWrite(ledPin, LOW);   // Turn off the LED
}
