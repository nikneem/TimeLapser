#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR    0x3F
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

const int INTERVAL_MAX_VALUE    = 60;
const int INTERVAL_MIN_VALUE    = 2;

const int PIN_CAMERA_FOCUS      = 11;
const int PIN_CAMERA_SHUTTER    = 12;

const int PIN_BUTTON_INCREASE   = 8;
const int PIN_BUTTON_DECREASE   = 9;
const int PIN_BUTTON_STARTSTOP  = 10;

long _interval = 5;
int _intervalAddress = 0;

unsigned long _millisSinceLastShot = 0;
unsigned long _lastShotMillis = 0;
unsigned long _lastDisplayUpdateMillis = 2500;

int lastPressedButton = 0;
int buttonRepeatInterval = 1500;
int buttonWaitDelay = 50;
int buttonRepeatCount = 0;

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

bool _isEnabled = false;
bool _isFocusMessaged = false;

void setup() {
  Serial.begin(57600);
  pinMode(PIN_CAMERA_FOCUS, OUTPUT);
  pinMode(PIN_CAMERA_SHUTTER, OUTPUT);
  int eepromValue = EEPROM.read(_intervalAddress);
  if (eepromValue >= 2 && eepromValue <= 60){
    _interval = eepromValue;
  }

  lcd.begin (16,2); //  <<----- My LCD was 16x2
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home (); // go home
  lcd.print("- Time Lapster -");  
  lcd.setCursor(0, 1);
  lcd.print("Loading...");
  delay(1500);
}

void loop() {
  _millisSinceLastShot = (millis() - _lastShotMillis);
  if ((millis() - _lastDisplayUpdateMillis) > 1000){
    _lastDisplayUpdateMillis = millis();
    UpdateDisplay();
    Serial.print("Time since last shot: ");
    Serial.println(_millisSinceLastShot);
  }
  
  if (_isEnabled){
    if ((_millisSinceLastShot > 250) && (_millisSinceLastShot < 1000)){
      CameraRelease();
    }
    if ((_millisSinceLastShot > 1000) && (_millisSinceLastShot < 1250)){
      UpdateInterval();
    }
    if (_millisSinceLastShot > ((_interval * 1000) - 1000)){
      CameraFocus();
    }
    if (_millisSinceLastShot > (_interval * 1000)){
      CameraTakePicture();
      _lastShotMillis = millis();
    }
  }

  if (digitalRead(PIN_BUTTON_INCREASE) == HIGH){
    IncreaseInterval();
    WaitForButtonRelease(PIN_BUTTON_INCREASE);
  }
  if (digitalRead(PIN_BUTTON_DECREASE) == HIGH){
    DecreaseInterval();
    WaitForButtonRelease(PIN_BUTTON_DECREASE);
  }
  if (digitalRead(PIN_BUTTON_STARTSTOP) == HIGH){
    _isEnabled = !_isEnabled;
    if (_isEnabled){
      _lastShotMillis = millis();
      _millisSinceLastShot = 0;
    }
    UpdateDisplay();
    WaitForButtonRelease(PIN_BUTTON_STARTSTOP);
  }


}

void CameraFocus(){
  digitalWrite(PIN_CAMERA_FOCUS, HIGH);
  if (!_isFocusMessaged){
    lcd.setCursor(0, 0);
  lcd.print("-  - FOCUS  -  -");
    _isFocusMessaged = true;
  }
}
void CameraTakePicture(){
  digitalWrite(PIN_CAMERA_SHUTTER, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("-  - CLICK  -  -");
  delay(250);
  digitalWrite(PIN_CAMERA_SHUTTER, LOW);  
  _isFocusMessaged = false;
}
void CameraRelease(){
  digitalWrite(PIN_CAMERA_FOCUS, LOW);
  digitalWrite(PIN_CAMERA_SHUTTER, LOW);
}

void WaitForButtonRelease(int pin){
  buttonRepeatCount = 0;
  // This method allows the user to press and hold the button
  // to increase, or decrease the value of the interval. After
  // holding it for 2 seconds, the interval value will be increased
  // or decreased every half second.
  if (pin != PIN_BUTTON_STARTSTOP){  
    if (lastPressedButton != pin){
      buttonRepeatInterval = 1500;
    } else {
      buttonRepeatInterval = 75;
    }
    
    // If a button is pressed, wait for the button
    // to be released, else the arduino loop would
    // see the button as pressed over and over again
    while ((digitalRead(pin) == HIGH) && ((buttonRepeatCount * buttonWaitDelay) < buttonRepeatInterval)){
      buttonRepeatCount++;
      delay(buttonWaitDelay);
    }
  } else {
    // This is not a repeatable button, so just wait
    // for the button release.    
    while (digitalRead(pin) == HIGH){
      buttonRepeatCount++;
      delay(buttonWaitDelay);
    }
  }
  lastPressedButton = pin;
}


int IncreaseInterval(){
  if (_interval < INTERVAL_MAX_VALUE){
    _interval++;
  }
  SaveIntervalSetting();
}
int DecreaseInterval(){
  if (_interval > INTERVAL_MIN_VALUE){
    _interval--; 
  }
  SaveIntervalSetting();
}
void SaveIntervalSetting(){
  EEPROM.write(_intervalAddress, _interval);
  Serial.print("Current interval ");
  Serial.println(_interval);
  UpdateInterval();
}

void UpdateDisplay(){
  String runningStatus = "TimeLapse Paused";
  if (_isEnabled){
    long intervalMillis = (_interval * 1000);
    long timeToNextPhotoMillis = intervalMillis - _millisSinceLastShot;
    long timeToNextPhoto = round(timeToNextPhotoMillis / 1000); // Add 400 to prevent nasty rounding differences
    runningStatus = "Next in " + String(timeToNextPhoto) + " sec.";
  }
  while (runningStatus.length() < 16){
    runningStatus += " ";
  }
 
  lcd.setCursor(0, 1);
  lcd.print(runningStatus);
}
void UpdateInterval(){
  String intervalSetting = "Interval " + String(_interval) + " sec.";
  while (intervalSetting.length() < 16){ intervalSetting += " "; }
  lcd.setCursor(0, 0);
  lcd.print(intervalSetting);
}
