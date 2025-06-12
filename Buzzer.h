
class Buzzer
{
private:
  byte buzzerPin;

public:
  Buzzer(byte buzzerPin)
      : buzzerPin(buzzerPin) {}

  void begin()
  {
    pinMode(buzzerPin, OUTPUT);
  }

  void beep()
  {
    tone(buzzerPin, 1000, 75);
  }

  void sos()
  {
    for (int i = 500; i <= 1800; i = i + 5)
    {
      tone(buzzerPin, i, 5);
    }
  }

  void success()
  {
    tone(buzzerPin, 1000, 75);
    delay(150);
    tone(buzzerPin, 1000, 75);
  }

  void failure()
  {
    tone(buzzerPin, 1000, 75);
    delay(150);
    tone(buzzerPin, 500, 75);
  }
};