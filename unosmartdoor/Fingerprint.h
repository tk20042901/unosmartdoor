
#include "Keypad.h"
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
class Fingerprint {
private:
  SoftwareSerial mySerial;
  Adafruit_Fingerprint finger;
  boolean firstImageSet = false;
public:
  Fingerprint(byte inPin, byte outPin)
    : mySerial(inPin, outPin), finger(&mySerial) {}

  void begin() {
    finger.begin(57600);
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