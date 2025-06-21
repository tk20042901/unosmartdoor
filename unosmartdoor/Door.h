#include <ESP32Servo.h>

class Door {
private:
  byte doorPin;
  Servo servo;

public:
  Door(byte doorPin)
    : doorPin(doorPin) {}

  void begin() {
    servo.attach(doorPin);
  }

  void lock() {
    servo.write(110);
  }

  void unlock(){
    servo.write(20);
  }
};
