class Buzzer {
private:
  byte buzzerPin;
  const int buzzerChannel = 3;
  const int buzzerResolution = 8;

  void playTone(int frequency, int duration) {
    ledcWriteTone(buzzerChannel, frequency);
    ledcWrite(buzzerChannel, 255);
    delay(duration);
    stopTone();
  }

  void stopTone() {
    ledcWrite(buzzerChannel, 0);
  }

public:
  Buzzer(byte buzzerPin)
    : buzzerPin(buzzerPin) {}

  void begin() {
    ledcSetup(buzzerChannel, 2000, buzzerResolution);
    ledcAttachPin(buzzerPin, buzzerChannel);
  }

  void beep() {
    playTone(1500, 100);
  }

  void success() {
    playTone(1200, 100);
    delay(120);
    playTone(1500, 100);
    delay(120);
    playTone(1800, 100);
    delay(120);
    stopTone();
  }

  void failure() {
    playTone(900, 150);
    delay(200);
    playTone(400, 300);
    delay(350);
    stopTone();
  }

  void sos() {
    for (int i = 500; i < 1800; i = i + 10) {
      playTone(i, 10);
    }
    stopTone();
  }
};
