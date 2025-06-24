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
byte oldState = -1;
byte state = LOCK_STATE;  //default state

//timeout in special state (seconds)
#define UNLOCK_TIMEOUT 5
#define CHANGE_PASSWORD_TIMEOUT 5
#define WAIT_TIMEOUT 15
#define ADD_FINGERPRINT_TIMEOUT 10
unsigned long expirationTime;

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


BLYNK_WRITE_DEFAULT() {
  int pin = request.pin;  // Lấy virtual pin được gửi tới
  int value = param.asInt();

  if (value == 0)
    return;  // Chỉ xử lý khi nhấn (tránh lặp lại khi thả nút)

  switch (pin) {
    case OPEN_DOOR_VIRTUAL_PIN:
      if (state == LOCK_STATE) {
        buzzer.success();
        lcd.display("Blynk opened", "Welcome back", 1);
        state = UNLOCK_STATE;
      }
      break;

    case CLEAR_FINGERPRINT_DATABASE_VIRTUAL_PIN:
      fingerprint.emptyDatabase();
      buzzer.success();
      Blynk.logEvent("notification", "Cleared fingerprint database");
      break;

    case STOP_SOS_STATE_VIRTUAL_PIN:
      if (state == SOS_STATE) {
        state = LOCK_STATE;
        Blynk.logEvent("notification", "SOS stopped");
      }
      break;

    case SHOW_PASSWORD_VIRTUAL_PIN:
      Blynk.logEvent("notification", "Your password is " + password);
      break;

    case CHANGE_PASSWORD_VIRTUAL_PIN:
      if (state != CHANGE_PASSWORD_STATE) {
        state = CHANGE_PASSWORD_STATE;
      }
      break;

    case ADD_FINGERPRINT_VIRTUAL_PIN:
      if (state != ADD_FINGERPRINT_STATE) {
        state = ADD_FINGERPRINT_STATE;
      }
      break;
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
  }  //default state
}

void loop() {
  Blynk.run();

  //init new state if state changed
  handleStateChange();

  setTimeOutForState();

  updateTimeOutWhenObstacle();

  checkTimeOut();

  readUserInput();
}

void handleStateChange() {
  if (oldState != state) {
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
void updateTimeOutWhenObstacle() {
  if (state == UNLOCK_STATE && ultrasonic.checkObstacle()) {
    setTimeOut(UNLOCK_STATE);
  }
}
void setTimeOutForState() {
  if (oldState == state)
    return;
  oldState = state;

  switch (state) {
    case UNLOCK_STATE:
      setTimeOut(UNLOCK_TIMEOUT);
      break;
    case ADD_FINGERPRINT_STATE:
      setTimeOut(ADD_FINGERPRINT_TIMEOUT);
      break;
    case CHANGE_PASSWORD_STATE:
      setTimeOut(CHANGE_PASSWORD_TIMEOUT);
      break;
  }
}

void checkTimeOut() {
  if (millis() < expirationTime) {
    return;
  }

  if (state == UNLOCK_STATE) {
    buzzer.beep();
    state = LOCK_STATE;
  } else if (state == CHANGE_PASSWORD_STATE || state == ADD_FINGERPRINT_STATE) {
    buzzer.beep();
    state = UNLOCK_STATE;
  }
}

void readUserInput() {
  if (state == LOCK_STATE) {
    checkFingerprint();
  } else if (state == ADD_FINGERPRINT_STATE) {
    addFingerprint();
  }

  char c = keypad.getKey();
  if (!c) return;

  buzzer.beep();

  switch (state) {
    case LOCK_STATE:
      keypadInputPassword(c);
      break;
    case ADD_FINGERPRINT_STATE:
      if (c == '#') {  //cancel
        state = UNLOCK_STATE;
      }
      break;
    case CHANGE_PASSWORD_STATE:
      keypadInputChangePassword(c);
      break;
  }
}

void checkFingerprint() {

  uint8_t p = fingerprint.getFingerprintIDez();

  if (p != FINGERPRINT_NOFINGER && fingerprintAttempt == MAX_ATTEMPT_FINGER) {  //if has fingerprint but fingerprint wrong too many times
    buzzer.failure();
    lcd.display("Fingerprint", "has been locked", 1);
    displayPassword();
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
    displayPassword();
  }
}

void addFingerprint() {
  if (!fingerprint.hadFirstImage()) {
    if (fingerprint.getFirstImage() == FINGERPRINT_OK) {
      buzzer.success();
      lcd.display("1st try success", "Remove finger", 1);
      lcd.display("Put finger", "again", 1);
      setTimeOut(ADD_FINGERPRINT_TIMEOUT);
    }
    return;
  }
  uint8_t result = fingerprint.addFinger();

  if (result == FINGERPRINT_OK) {
    buzzer.success();
    lcd.display("Add fingerprint", "success", 1);
    fingerprint.resetHadFirstImage();
    state = UNLOCK_STATE;
  } else if (result != FINGERPRINT_NOFINGER) {
    buzzer.failure();
    lcd.display("2nd try not", "match 1st", 1);
  }
}

void lockState() {
  door.lock();
  inputPassword = "";
  displayPassword();
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "Locked");
}

void unlockState() {
  door.unlock();
  passwordAttempt = 0;
  fingerprintAttempt = 0;
  lcd.display("Welcome back");
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "Unlock");
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
void changePasswordState() {
  inputChangePassword = "";
  displayPassword();
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "Changing password");
}

void addFingerprintState() {
  if (fingerprint.isFullData()) {
    Blynk.logEvent("notification", "Can't add new fingerprint because database is full");
    state == UNLOCK_STATE;
    return;
  }
  buzzer.beep();
  lcd.display("Put your finger");
  Blynk.virtualWrite(STATE_VIRTUAL_PIN, "Adding fingerprint");
  //ensure
  fingerprint.resetHadFirstImage();
}

void keypadInputPassword(char c) {
  if (c == '#') {
    if (inputPassword.length() > 0) {
      inputPassword.remove(inputPassword.length() - 1);
    }
  } else if (isDigit(c)) {
    if (inputPassword.length() < 4) {
      inputPassword += c;
    }
  }

  displayInputPassword();

  if (inputPassword.length() == 4) {
    checkPassword();
  }
}

void checkPassword() {
  if (inputPassword == password) {
    buzzer.success();
    lcd.display("Correct password", "Welcome back", 1);
    state = UNLOCK_STATE;
  } else {
    buzzer.failure();
    passwordAttempt++;
    inputPassword = "";

    if (passwordAttempt == MAX_ATTEMPT_PASSWORD - 1) {
      wait();
    } else if (passwordAttempt >= MAX_ATTEMPT_PASSWORD) {
      state = SOS_STATE;
    } else {
      lcd.display("Wrong password", 1);
    }
  }
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
  displayPassword();
}


void keypadInputChangePassword(char c) {
  if (c == '*') {  // Submit
    if (inputChangePassword.length() != 4) {
      buzzer.failure();
      lcd.display("Password must", "have 4 digits", 1);
    } else {
      buzzer.success();
      password = inputChangePassword;
      lcd.display("Password changed", "successfully", 1);
      state = UNLOCK_STATE;
    }
  } else if (c == '#') {  // Backspace
    if (inputChangePassword.length() > 0) {
      inputChangePassword.remove(inputChangePassword.length() - 1);
    }
  } else if (isDigit(c)) {
    if (inputChangePassword.length() < 4) {
      inputChangePassword += c;
    } else {
      lcd.display("Max 4 digits", "", 1);
    }
  }
  displayPassword();
}

void displayPassword() {
  if (fingerprintAttempt != MAX_ATTEMPT_FINGER && state == LOCK_STATE) {
    lcd.display("Password/Finger", code.substring(0, inputPassword.length()));
  } else if (fingerprintAttempt == MAX_ATTEMPT_FINGER && state == LOCK_STATE) {
    lcd.display("Input password", code.substring(0, inputPassword.length()));
  } else if (state == CHANGE_PASSWORD_STATE) {
    lcd.display("Enter new", "password:" + code.substring(0, inputChangePassword.length()));
  }
}
void setTimeOut(unsigned long second) {
  timeOutDeadline = millis() + second * 1000;
}
