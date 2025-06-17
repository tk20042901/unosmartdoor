#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

class Fingerprint {
private:
  HardwareSerial mySerial;
  Adafruit_Fingerprint finger;
  byte inPin, outPin;
  boolean firstImageSet = false;

public:
  Fingerprint(byte inPin, byte outPin)
    : mySerial(1), finger(&mySerial), inPin(inPin), outPin(outPin) {}

  void begin() {
    mySerial.begin(57600, SERIAL_8N1, inPin, outPin);
  }

  uint8_t getFingerprintIDez() {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK)
      return p;
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK)
      return p;
    p = finger.fingerFastSearch();
    return p;
  }

  uint8_t getFirstImage() {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) {
      return p;
    }
    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK) {
      return p;
    }
    firstImageSet = true;
    return p;
  }

  uint8_t addFinger(uint8_t id = 1) {
    uint8_t p;

    p = finger.getImage();
    if (p != FINGERPRINT_OK) {
      return p;
    }

    p = finger.image2Tz(2);
    if (p == FINGERPRINT_OK) {
      return p;
    }

    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
      return p;
    }

    p = finger.storeModel(id);
    if (p != FINGERPRINT_OK) {
      return p;
    }
    firstImageSet = false;
    return p;
  }

  boolean verifyPassword() {
    return finger.verifyPassword();
  }
  
  boolean hadFirstImage() {
    return firstImageSet;
  }
};