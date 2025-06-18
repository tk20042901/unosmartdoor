class Buzzer {
private:
  byte buzzerPin;

public:
  Buzzer(byte buzzerPin)
    : buzzerPin(buzzerPin) {}

  void begin() {
    pinMode(buzzerPin, OUTPUT);
  }

  void beep() {
    tone(buzzerPin, 1500, 100);
    delay(120);
    noTone(buzzerPin);
  }

  void sos() {
    int shortTone = 200;
    int longTone = 600;
    int pause = 150;

    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 1000, shortTone);
      delay(shortTone + pause);
    }

    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 1000, longTone);
      delay(longTone + pause);
    }

    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 1000, shortTone);
      delay(shortTone + pause);
    }

    noTone(buzzerPin);
  }

  void success() {
    tone(buzzerPin, 1200, 100);
    delay(120);
    tone(buzzerPin, 1500, 100);
    delay(120);
    tone(buzzerPin, 1800, 100);
    delay(120);
    noTone(buzzerPin);
  }

  void failure() {
    tone(buzzerPin, 900, 150);
    delay(200);
    tone(buzzerPin, 400, 300);
    delay(350);
    noTone(buzzerPin);
  }
};
