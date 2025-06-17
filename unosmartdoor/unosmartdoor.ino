#include "LCD.h"
#include "Ultrasonic.h"
#include "Buzzer.h"
#include "Fingerprint.h"
#include <Keypad.h>
#include <ESP32Servo.h>

// lcd 1602
#define SDA_PIN 22
#define SCL_PIN 23
LCD lcd(SDA_PIN, SCL_PIN);

// keypad
char keys[4][3] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
#define KEYPAD_R1_PIN 27
#define KEYPAD_R2_PIN 26
#define KEYPAD_R3_PIN 25
#define KEYPAD_R4_PIN 33
#define KEYPAD_C1_PIN 32
#define KEYPAD_C2_PIN 35
#define KEYPAD_C3_PIN 34
byte rowPins[4] = { KEYPAD_R1_PIN, KEYPAD_R2_PIN, KEYPAD_R3_PIN, KEYPAD_R4_PIN };
byte colPins[3] = { KEYPAD_C1_PIN, KEYPAD_C2_PIN, KEYPAD_C3_PIN };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 3);

// servo
#define SERVO_PIN 15
Servo servo;

// ultrasonic distance sensor
#define TRIG_PIN 2
#define ECHO_PIN 4
Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);

// buzzer
#define BUZZER_PIN 5
Buzzer buzzer(BUZZER_PIN);
// fingerprint
#define F_IN_PIN 33
#define F_OUT_PIN 32
Fingerprint finger(F_IN_PIN, F_OUT_PIN);
// states
#define LOCK_STATE 1
#define UNLOCK_STATE 2
#define ADD_FINGERPRINT_STATE 3
#define CHANGE_PASSWORD_STATE 4
#define WAIT_STATE 5
#define SOS_STATE 6
byte oldState = LOCK_STATE;
byte state = LOCK_STATE;  // default state

// timeout in special state (seconds)
#define UNLOCK_TIMEOUT 10
#define CHANGE_PASSWORD_TIMEOUT 5
#define WAIT_TIMEOUT 5
#define MAX_ATTEMPT_FINGER 3
#define MAX_TIME_FINGERPRINT 20000
unsigned long timeOutTimer;

// password
String password = "0000";  // default password
String inputPassword;
byte limitWrongTime = 3;
byte fpAttempt = 0;
byte wrongPasswordTime;
String inputChangePassword = "";
const String code = "****";

void setup() {
  lcd.begin();
  servo.attach(SERVO_PIN);
  servo.write(180);
  ultrasonic.begin();
  buzzer.begin();
  finger.begin();
  while (!finger.verifyPassword()) {
    buzzer.failure();
    lcd.display("fing error");
  }
  passwordState();  // default state
}

void loop() {
  // change display if state changed

  handleStateChange();

  // check timeout in special state
  checkTimeOut();

  readFingerInput();

  readKeypadInput();
}

void readFingerInput() {
  if (state == LOCK_STATE) {
    handleLockStateFingerprint();
  } else if (state == ADD_FINGERPRINT_STATE) {
    handleAddFingerprintState();
  }
}

void handleLockStateFingerprint() {
  if (fpAttempt >= MAX_ATTEMPT_FINGER) {
    buzzer.failure();
    lcd.display("You can not use fingerprint", "", 1);
    return;
  }

  uint8_t p = finger.getFingerprintIDez();

  if (p == FINGERPRINT_OK) {
    buzzer.success();
    lcd.display("Correct password", "Welcome back", 2);
    state = UNLOCK_STATE;
  } else if (p != FINGERPRINT_NOFINGER) {
    fpAttempt++;
    buzzer.failure();
    lcd.display("Wrong finger", "Attempt: " + String(fpAttempt), 1);

    if (fpAttempt >= MAX_ATTEMPT_FINGER) {
      lcd.display("Fingerprint", "has been locked", 1);
    }
  }

  delay(100);
}
void handleAddFingerprintState() {
  if (!finger.hadFirstImage()) {
    if (finger.getFirstImage() == FINGERPRINT_OK) {
      buzzer.success();
      lcd.display("1st success", "Remove finger", 2);
      delay(2000);
      lcd.display("Put finger", "again");
    }
  } else {
    uint8_t p = finger.addFinger();
    if (p == FINGERPRINT_OK) {
      buzzer.success();
      lcd.display("Add fingerprint success", "", 1);
      state = UNLOCK_STATE;
    } else if (p != FINGERPRINT_NOFINGER) {
      buzzer.failure();
      lcd.display("2nd finger not", "equal to 1st", 1);
    }
  }
}


void handleStateChange() {
  if (oldState != state) {
    oldState = state;
    switch (state) {
      case LOCK_STATE:
        passwordState();
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
      case WAIT_STATE:
        waitState();
        break;
      case SOS_STATE:
        SOS_State();
        break;
    }
  }
}

void checkTimeOut() {
  if (state == UNLOCK_STATE) {
    if (ultrasonic.checkObstacle()) {  // check for obstacle in front of door
      setTimeOut(UNLOCK_TIMEOUT);
    } else if (timeOutTimer < millis()) {
      buzzer.beep();
      state = LOCK_STATE;
    }
  }
  if (state == CHANGE_PASSWORD_STATE || state == ADD_FINGERPRINT_STATE) {
    if (timeOutTimer < millis()) {
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
        runPassword(c);
        break;
      case UNLOCK_STATE:
        whenunlock(c);
        break;
      case ADD_FINGERPRINT_STATE:
        runAddFingerprint(c);
        break;
      case CHANGE_PASSWORD_STATE:
        changePassword(c);
        break;
    }
  }
}

void passwordState() {
  // turn servo
  servo.write(180);
  inputPassword = "";
  displayInputPassword();
}

void unlockState() {
  servo.write(90);
  setTimeOut(UNLOCK_TIMEOUT);
  wrongPasswordTime = 0;
  fpAttempt = 0;
  lcd.display("(*)+ Fingerprint", "(#)Change pass");
}

void changePasswordState() {
  inputChangePassword = "";
  displayInputChangePassword();
}

void waitState() {
  lcd.display("Wrong password", "too many times", 1);
  lcd.display("Try again in ");
  for (byte timer = WAIT_TIMEOUT; timer > 0; timer--) {
    lcd.setCursor(0, 1);
    lcd.print(timer);
    lcd.print(" seconds   ");
    delay(1000);
  }
  state = LOCK_STATE;
}
void addFingerprintState() {
  setTimeOut(MAX_TIME_FINGERPRINT);
  buzzer.beep();
  lcd.display("Put your finger");
}
void SOS_State() {
  lcd.display("Khong lam ma doi", "co an thi an ...");
  while (true) {
    buzzer.sos();
  }
}

void setTimeOut(byte time) {
  timeOutTimer = millis() + time * 1000;
}

void whenunlock(char c) {
  if (c == '*') {
    state = ADD_FINGERPRINT_STATE;
  }
  if (c == '#') {
    state = CHANGE_PASSWORD_STATE;
  }
}

void runPassword(char c) {
  if (c == '#') {  // backspace
    inputPassword = inputPassword.substring(0, inputPassword.length() - 1);
  } else if (isDigit(c)) {  // only input number
    inputPassword += c;
  }
  if (inputPassword.length() == 4) {       // after input 4 digit, check correct immediately
    if (inputPassword.equals(password)) {  // correct
      buzzer.success();
      lcd.display("Correct password", "Welcome back", 1);
      state = UNLOCK_STATE;
      return;
    } else {  // wrong
      buzzer.failure();
      wrongPasswordTime++;
      if (wrongPasswordTime == limitWrongTime) {  // when password wrong many times
        state = WAIT_STATE;
        return;
      }
      if (wrongPasswordTime > limitWrongTime) {  // when password wrong after wait state
        state = SOS_STATE;
        return;
      }
      inputPassword = "";
      lcd.display("Wrong password", "", 1);
    }
  }
  displayInputPassword();
}

void displayInputPassword() {
  lcd.display("Keypad or (*)Fing", code.substring(0, inputPassword.length()));
}

void changePassword(char c) {
  if (c == '*') {                             // submit
    if (inputChangePassword.length() != 4) {  // password must have 4 digit
      buzzer.failure();
      lcd.display("Password must", "have 4 digit", 1);
    } else {  // change password successfully
      buzzer.success();
      password = inputChangePassword;
      lcd.display("Change password", "successfully", 1);
      state = UNLOCK_STATE;
      return;
    }
  } else if (c == '#') {  // backspace
    inputChangePassword = inputChangePassword.substring(0, inputChangePassword.length() - 1);
  } else if (inputChangePassword.length() == 4) {  // password must have 4 digit
    lcd.display("Too long input", "", 1);
  } else if (isDigit(c)) {
    inputChangePassword += c;
  }
  displayInputChangePassword();
}

void displayInputChangePassword() {
  setTimeOut(CHANGE_PASSWORD_TIMEOUT);
  lcd.display("Enter new", "password:" + code.substring(0, inputChangePassword.length()));
}

void runAddFingerprint(char c) {
  if (c == '#') {  // cancel
    state = UNLOCK_STATE;
  }
}
