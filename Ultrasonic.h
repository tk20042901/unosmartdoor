class Ultrasonic {
private:
  byte trigPin;
  byte echoPin;

public:
  Ultrasonic(byte trigPin, byte echoPin)
    : trigPin(trigPin), echoPin(echoPin) {}

  void begin() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
  }

  bool checkObstacle() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long distance = 0.01723 * pulseIn(echoPin, HIGH);
    return distance < 10;
  }
};
