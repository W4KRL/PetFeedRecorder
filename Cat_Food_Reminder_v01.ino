// Cat_Food_Reminder_v01.ino
//
// Display time pet is fed breakfast, lunch, dinner and snack.
// Uses library for DS1307 RTC commonly found on eBay
// Could use DS3231 with appropriate library
// Liquid Crystal Display is common 1602 type with I2C adapter
//
// Karl Berger, W4KRL
// December 3, 2015
// restored averaging to filter button value
//
// Enhancements:
//  Store status in PRGMEN
//  Improve wrong entry response
//  for example, use default times
//  Add splash screen that asks for confirmation of time
//  Add time setting function

#include <Wire.h>                 // I2C library built-in    
#include <RTClib.h>               // https://github.com/adafruit/RTClib
#include <LiquidCrystal_I2C.h>    // https://github.com/marcoschwartz/LiquidCrystal_I2C.git

// instantiate real time clock
RTC_DS1307 rtc;

// instantiate LCD display
// lcd(I2C address, columns, rows)
// your LCD may have a different I2C address - 0x20 is common
LiquidCrystal_I2C lcd(0x27, 16, 2);

// define constants used by the panel and buttons
const char petName[] = "Penny";
const char petGender[] = "her";    // "her or "his"
const int btnBREAKFAST = 0;
const int btnLUNCH = 1;
const int btnDINNER = 2;
const int btnSNACK = 3;
//const int btnSpare = 4;
const int btnNONE = 5;

// right or wrong button press
const int OK = 0;
const int BAD = 1;

// musical notes
const int c = 3830;    // 261 Hz
const int d = 3400;    // 294 Hz
const int e = 3038;    // 329 Hz
const int f = 2864;    // 349 Hz
const int g = 2550;    // 392 Hz
const int a = 2272;    // 440 Hz
const int b = 2028;    // 493 Hz
const int C = 1912;    // 523 Hz
// Define a special note, 'R', to represent a rest
const int R = 0;

// IO connections
const int buttonPin = A0;
const int speakerPin = 5;

// global variables
int lastButton = btnSNACK;

//soft kitty, warm kitty
int melody[] = { g,  e,  e,  f,  d,  d,  c,  d,  e,  f,  g,  R,  g,  g,  e,  e,  f,  f,  d,  d,  c,  d,  c};
int beats[] =  {64, 16, 16, 64, 16, 16, 16, 16, 16, 16, 64, 100, 16, 16, 16, 16, 16, 16, 16, 16, 64, 64, 128};
int MAX_COUNT = sizeof(melody) / 2; // Melody length, for looping.

// Set overall tempo
const long tempo = 8000;
// Set length of pause between notes
const int pause = 150;
// Loop variable to increase Rest length
const int rest_count = 20;

// Initialize core variables
int tone_ = 0;
int beat = 0;
long duration = 0;

// ******************** SETUP ***********************
// start all devices
void setup() {
  Serial.begin(9600);
  Wire.begin();        // start I2C
  rtc.begin();         // start the real time clock

  // comment out next line after clock has been set
  //  rtc.adjust(DateTime(__DATE__, __TIME__));

  lcd.init();          // start the LCD
  lcd.backlight();  
  lcd.print("   Feed ");
  lcd.print(petName);
  lcd.print('!');
  
  DateTime now = rtc.now();
  lcd.setCursor(0, 1);
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.day(), DEC);
  lcd.print('/');
  lcd.print(now.year(), DEC);
  lcd.print(" ");
  lcd.print(now.hour(), DEC);
  printDigits(now.minute());

  pinMode(speakerPin, OUTPUT);
  playSong();
} //setup()
// *********************SETUP END ************************

// ********************** LOOP ***************************
// continually scan buttons and display meal fed
void loop() {

  int buttonPressed = readButtons();   // read the buttons

  switch (buttonPressed) {
    case btnBREAKFAST:
      {
        if (lastButton == btnSNACK) {
          printTimeStamp();
          lcd.print("BREAKFAST.");
          alert(OK);
          lastButton = btnBREAKFAST;
        } else {
          printError();
        }
        break;
      }
    case btnLUNCH:
      {
        if (lastButton == btnBREAKFAST) {
          printTimeStamp();
          lcd.print(petGender);
          lcd.print(" LUNCH.");
          alert(OK);
          lastButton = btnLUNCH;
        } else {
          printError();
        }
        break;
      }
    case btnDINNER:
      {
        if (lastButton == btnLUNCH) {
          printTimeStamp();
          lcd.print(petGender);
          lcd.print(" DINNER.");
          alert(OK);
          lastButton = btnDINNER;
        } else {
          printError();
        }
        break;
      }
    case btnSNACK:
      {
        if (lastButton == btnDINNER) {
          printTimeStamp();
          lcd.print(petGender);
          lcd.print(" SNACK. ");
          alert(OK);
          lastButton = btnSNACK;
        } else {
          printError();
        }
        break;
      }
    default:
      break;
  }
} // loop()
// ********************** LOOP END ************************

// read the button voltage and convert to button ID
int readButtons() {
  int tempValue = 1023;
  int buttonPinValue = analogRead(buttonPin);

  if (buttonPinValue < 1000) {
    for (int j = 0; j < 4; j++) {
      delay(1);
      buttonPinValue = buttonPinValue + analogRead(buttonPin);
    }
    buttonPinValue = buttonPinValue / 5;

    // loop until button is released
    do {
      delay(50);
      tempValue = analogRead(buttonPin);
    } while (tempValue < 1000);
  }

  //------------------
  // my buttons read 0, 206, 391, 603, and 1023 (none)
  // add approx 50 to those values and check to see if we are close

  if (buttonPinValue > 900) return btnNONE;         // 1023
  if (buttonPinValue < 50)  return btnBREAKFAST;    //    0
  if (buttonPinValue < 250) return btnLUNCH;        //  206
  if (buttonPinValue < 450) return btnDINNER;       //  391
  if (buttonPinValue < 650) return btnSNACK;        //  603

  return btnNONE;                // when all others fail, return this.
} // readButtons()

// print leading 0 for time digits less than 10
int printDigits(int digits) {
  lcd.print(':');
  if (digits < 10) {
    lcd.print('0');
  }
  lcd.print(digits);
} // printDigits()

void printTimeStamp() {
  DateTime now = rtc.now();
  lcd.setCursor(0, 0);
  lcd.print("At ");

  if (now.hour() > 12) {
    lcd.print(now.hour() - 12);
  } else {
    lcd.print(now.hour());
  }

  printDigits(now.minute());

  if (now.hour() < 12) {
    lcd.print('a');
  } else {
    lcd.print('p');
  }
  lcd.print(" ");
  lcd.print(petName);
  lcd.print("   ");
  lcd.setCursor(0, 1);
  lcd.print("had ");
} // printTimeStamp()

void printError() {
  lcd.clear();
  lcd.print(petName);
  lcd.print(" wants ");
  lcd.print(petGender);
  lcd.setCursor(0, 1);
  switch (lastButton) {
    case btnSNACK:
      lcd.print("breakfast.");
      break;
    case btnBREAKFAST:
      lcd.print("lunch.");
      break;
    case btnLUNCH:
      lcd.print("dinner.");
      break;
    case btnDINNER:
      lcd.print("snack.");
      break;
  }
  alert(BAD);
} //printError()

// make a sound
void alert(int status) {
  if (status == OK) {
    playSong();
  } else {  // not OK or BAD
    tone(speakerPin, 3000);
    delay(100);
    noTone(speakerPin);
    tone(speakerPin, 2000);
    delay(100);
    noTone(speakerPin);
  }
} // alert()

void playSong() {
  // Set up a counter to pull from melody[] and beats[]
  for (int i = 0; i < MAX_COUNT; i++) {
    tone_ = melody[i];
    beat = beats[i];

    duration = beat * tempo; // Set up timing

    playTone();
    delay(pause);
  }
} // playSong()

// PLAY TONE  ==============================================
// Pulse the speaker to play a tone for a particular duration
void playTone() {
  long elapsed_time = 0;
  if (tone_ > 0) { // if this isn't a Rest beat, while the tone has
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duration) {

      digitalWrite(speakerPin, HIGH);
      delayMicroseconds(tone_ / 2);

      // DOWN
      digitalWrite(speakerPin, LOW);
      delayMicroseconds(tone_ / 2);

      // Keep track of how long we pulsed
      elapsed_time += (tone_);
    }
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { // See NOTE on rest_count
      delayMicroseconds(duration);
    }
  }
} // playTone()

