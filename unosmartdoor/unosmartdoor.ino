#include "LCD.h"
#include "Ultrasonic.h"
#include "Buzzer.h"
#include "Fingerprint.h"
#include <Keypad.h>
#include "Door.h"

//lcd 1602
//SCL connect to pin 22 in ESP32 (SCL pin)
//SDA connect to pin 21 in ESP32 (SDA pin)
LCD lcd;

//keypad
char keys[4][3] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
#define KEYPAD_R1_PIN 13
#define KEYPAD_R2_PIN 12
#define KEYPAD_R3_PIN 14
#define KEYPAD_R4_PIN 27
#define KEYPAD_C1_PIN 26
#define KEYPAD_C2_PIN 25
#define KEYPAD_C3_PIN 33
byte rowPins[4] = { KEYPAD_R1_PIN, KEYPAD_R2_PIN, KEYPAD_R3_PIN, KEYPAD_R4_PIN };
byte colPins[3] = { KEYPAD_C1_PIN, KEYPAD_C2_PIN, KEYPAD_C3_PIN };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 3);

//door
#define DOOR_PIN 13
Door door(DOOR_PIN);

//ultrasonic distance sensor
#define TRIG_PIN 2
#define ECHO_PIN 4
Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);

//buzzer
#define BUZZER_PIN 5
Buzzer buzzer(BUZZER_PIN);

//fingerprint
//TX connect to pin RX2 in ESP32
//RX connect to pin TX2 in ESP32
Fingerprint fingerprint;

//states
#define LOCK_STATE 1
#define UNLOCK_STATE 2
#define ADD_FINGERPRINT_STATE 3
#define CHANGE_PASSWORD_STATE 4
#define SOS_STATE 5
byte oldState = LOCK_STATE;
byte state = LOCK_STATE;  //default state

//timeout in special state (seconds)
#define UNLOCK_TIMEOUT 5
#define CHANGE_PASSWORD_TIMEOUT 5
#define WAIT_TIMEOUT 15
#define ADD_FINGERPRINT_TIMEOUT 10
unsigned long timeOutTimer;

//lock state
String password = "0000";  //default password
String inputPassword;
byte fingerprintAttempt = 0;
byte passwordAttempt = 0;
#define MAX_ATTEMPT_PASSWORD 5
#define MAX_ATTEMPT_FINGER 3
String inputChangePassword = "";
const String code = "****";

//blynk
#define BLYNK_TEMPLATE_ID "TMPL6vTL9tN0m"
#define BLYNK_TEMPLATE_NAME "Smart Door"
#define BLYNK_AUTH_TOKEN "GdD3gW1OxFmiT9WUxELT5yF5iXM3HYQo"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

//wifi info
char ssid[] = "2910A";
char pass[] = "0974040555";

//blynk virtual pins
#define OPEN_DOOR_VIRTUAL_PIN V0
#define CLEAR_FINGERPRINT_DATABASE_VIRTUAL_PIN V1
#define STOP_SOS_STATE_VIRTUAL_PIN V2
#define SHOW_PASSWORD_VIRTUAL_PIN V3
#define CHANGE_PASSWORD_VIRTUAL_PIN V4
#define ADD_FINGERPRINT_VIRTUAL_PIN V5
#define STATE_VIRTUAL_PIN V6

BLYNK_WRITE(OPEN_DOOR_VIRTUAL_PIN) {
  if (param.asInt()) {
    if (state == LOCK_STATE) {
      buzzer.success();
      lcd.display("Blynk opened", "Welcome back", 1);
      state = UNLOCK_STATE;
    }
  }
}

BLYNK_WRITE(CLEAR_FINGERPRINT_DATABASE_VIRTUAL_PIN) {
  if (param.asInt()) {
    fingerprint.emptyDatabase();
    buzzer.success();
    Blynk.logEvent("notification", "Cleared fingerprint database");
  }
}

BLYNK_WRITE(STOP_SOS_STATE_VIRTUAL_PIN) {
  if (param.asInt()) {
    if (state == SOS_STATE) {
      Blynk.logEvent("notification", "SOS stoped");
      state = LOCK_STATE;
    }
  }
}

BLYNK_WRITE(SHOW_PASSWORD_VIRTUAL_PIN) {
  if (param.asInt()) {
    Blynk.logEvent("notification", "Your password is " + password);
  }
}

BLYNK_WRITE(CHANGE_PASSWORD_VIRTUAL_PIN) {
  if (param.asInt()) {
    if (state != CHANGE_PASSWORD_STATE)
      state = CHANGE_PASSWORD_STATE;
  }
}

BLYNK_WRITE(ADD_FINGERPRINT_VIRTUAL_PIN) {
  if (param.asInt()) {
    if (state != ADD_FINGERPRINT_STATE)
      state = ADD_FINGERPRINT_STATE;
  }
}

void setup() {
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.begin();
  door.begin();
  ultrasonic.begin();
  buzzer.begin();
  fingerprint.begin();
  while (!fingerprint.verifyPassword()) {
    buzzer.failure();
    lcd.display("Can't connect", "to fingerprint");
  }
  lockState();  //default state
}

void loop() {
  Blynk.run();

  //init new state if state changed
  handleStateChange();

  //check timeout in special state
  checkTimeOut();

  readFingerInput();

  readKeypadInput();
}

void handleStateChange() {
  if (oldState != state) {
    oldState = state;
    switch (state) {
      case LOCK_STATE:
        lockState();
        break;
      case UNLOCK_STATE:
        unlockState();
        break;
      case ADD_FINGERPRINT_STATE:
        addFingerprintState();
        break;
      case CHANGE_PASSWORD_STATE:
        changePasswordState();
        break;
      case SOS_STATE:
        SOS_State();
        break;
    }
  }
}

void checkTimeOut() {
  if (state == UNLOCK_STATE) {
    if (ultrasonic.checkObstacle()) {  //check for obstacle in front of door
      setTimeOut(UNLOCK_TIMEOUT);
    } else if (timeOutTimer < millis()) {
      buzzer.beep();
      state = LOCK_STATE;
      return;
    }
  }
  if (state == CHANGE_PASSWORD_STATE || state == ADD_FINGERPRINT_STATE) {
    if (timeOutTimer < millis()) {
      buzzer.beep();
      state = UNLOCK_STATE;
      return;
    }
  }
}

void readFingerInput() {
  if (state == LOCK_STATE) {
    handleLockStateFingerprint();
  } else if (state == ADD_FINGERPRINT_STATE) {
    handleAddFingerprintState();
  }
}

void handleLockStateFingerprint() {

  uint8_t p = fingerprint.getFingerprintIDez();

  if (p != FINGERPRINT_NOFINGER && fingerprintAttempt == MAX_ATTEMPT_FINGER) {  //if has fingerprint but fingerprint wrong too many times
    buzzer.failure();
    lcd.display("Fingerprint", "has been locked", 1);
    displayInputPassword();
    return;
  }

  if (p == FINGERPRINT_OK) {  //valid finger
    buzzer.success();
    lcd.display("Valid finger", "Welcome back", 1);
    state = UNLOCK_STATE;
    return;
  } else if (p != FINGERPRINT_NOFINGER) {  //wrong finger
    fingerprintAttempt++;
    buzzer.failure();
    lcd.display("Wrong finger", 1);
    if (fingerprintAttempt == MAX_ATTEMPT_FINGER) {  //when fingerprint wrong too many times
      lcd.display("Wrong finger", "too many times", 1);
      lcd.display("Fingerprint", "has been locked", 1);
    }
    displayInputPassword();
  }
}

void handleAddFingerprintState() {
  if (!fingerprint.hadFirstImage()) {
    if (fingerprint.getFirstImage() == FINGERPRINT_OK) {
      buzzer.success();
      lcd.display("1st try success", "Remove finger", 1);
      lcd.display("Put finger", "again", 1);
      setTimeOut(ADD_FINGERPRINT_TIMEOUT);
    }
  } else {
    uint8_t p = fingerprint.addFinger();
    if (p == FINGERPRINT_OK) {
      buzzer.success();
      lcd.display("Add fingerprint", "success", 1);
      fingerprint.resetHadFirstImage();
      state = UNLOCK_STATE;
    } else if (p != FINGERPRINT_NOFINGER) {
      buzzer.failure();
      lcd.display("2nd try not", "match 1st", 1);
      fingerprint.resetHadFirstImage();
      state = UNLOCK_STATE;
    }
  }
}

void readKeypadInput() {
  char c = keypad.getKey();
  if (c) {
    buzzer.beep();
    switch (state) {
      case LOCK_STATE:
        keypadInputPassword(c);
        break;
      case ADD_FINGERPRINT_STATE:
        keypadInputAddFingerprint(c);
        break;
      case CHANGE_PASSWORD_STATE:
        keypadInputChangePassword(c);
        break;
    }
  }
}

void lockState() {
  door.lock();
  inputPassword = "";
  displayInputPassword();
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "Locking");
}

void unlockState() {
  door.unlock();
  setTimeOut(UNLOCK_TIMEOUT);
  passwordAttempt = 0;
  fingerprintAttempt = 0;
  lcd.display("Welcome back");
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "Unlocked");
}

void changePasswordState() {
  inputChangePassword = "";
  displayInputChangePassword();
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "Changing password");
}

void addFingerprintState() {
  if (fingerprint.isFullData()) {
    Blynk.logEvent("notification", "Can't add new fingerprint because database is full");
    state == UNLOCK_STATE;
    return;
  }
  setTimeOut(ADD_FINGERPRINT_TIMEOUT);
  buzzer.beep();
  lcd.display("Put your finger");
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "Adding fingerprint");
}

void SOS_State() {
  lcd.display("Door is locked");
  Blynk.logEvent("sos");
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "SOS!!!");
  while (state == SOS_STATE) {
    buzzer.sos();
    Blynk.run();  //check stop sos in blynk
  }
}

void setTimeOut(byte time) {
  timeOutTimer = millis() + time * 1000;
}

void keypadInputPassword(char c) {
  if (c == '#') {  //backspace
    inputPassword = inputPassword.substring(0, inputPassword.length() - 1);
  } else if (isDigit(c)) {  //only input number
    inputPassword += c;
  }
  if (inputPassword.length() == 4) {       //after input 4 digit, check correct immediately
    if (inputPassword.equals(password)) {  //correct
      buzzer.success();
      lcd.display("Correct password", "Welcome back", 1);
      state = UNLOCK_STATE;
      return;
    } else {  //wrong
      buzzer.failure();
      passwordAttempt++;
      inputPassword = "";
      if (passwordAttempt == MAX_ATTEMPT_PASSWORD - 1) {  //when password wrong too many times
        wait();
        return;
      }
      if (passwordAttempt >= MAX_ATTEMPT_PASSWORD) {  //when password wrong after wait
        state = SOS_STATE;
        return;
      }
      lcd.display("Wrong password", 1);
    }
  }
  displayInputPassword();
}

void wait() {
  lcd.display("Wrong password", "too many times", 1);
  lcd.display("Try again in ");
  for (byte timer = WAIT_TIMEOUT; timer > 0; timer--) {
    lcd.setCursor(0, 1);
    lcd.print(timer);
    lcd.print(" seconds   ");
    delay(1000);
  }
  displayInputPassword();
}

void displayInputPassword() {
  if (fingerprintAttempt != MAX_ATTEMPT_FINGER) {
    lcd.display("Password/Finger", code.substring(0, inputPassword.length()));
  } else {
    lcd.display("Input password", code.substring(0, inputPassword.length()));
  }
}

void keypadInputChangePassword(char c) {
  if (c == '*') {                             //submit
    if (inputChangePassword.length() != 4) {  //password must have 4 digit
      buzzer.failure();
      lcd.display("Password must", "have 4 digit", 1);
    } else {  //change password successfully
      buzzer.success();
      password = inputChangePassword;
      lcd.display("Change password", "successfully", 1);
      state = UNLOCK_STATE;
      return;
    }
  } else if (c == '#') {  //backspace
    inputChangePassword = inputChangePassword.substring(0, inputChangePassword.length() - 1);
  } else if (inputChangePassword.length() == 4) {  //password must have 4 digit
    lcd.display("Too long input", "", 1);
  } else {
    inputChangePassword += c;
  }
  displayInputChangePassword();
}

void displayInputChangePassword() {
  setTimeOut(CHANGE_PASSWORD_TIMEOUT);
  lcd.display("Enter new", "password:" + code.substring(0, inputChangePassword.length()));
}

void keypadInputAddFingerprint(char c) {
  if (c == '#') {  //cancel
    state = UNLOCK_STATE;
  }
}
