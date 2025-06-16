
#include "Keypad.h"
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
class Fingerprint {
private:
  SoftwareSerial mySerial;
  Adafruit_Fingerprint finger;

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


  bool getFirstImage(ulong timeout) {
    ulong start = millis();
    while (millis() - start < timeout) {
      uint8_t p = finger.getImage();
      if (p != FINGERPRINT_OK) {
        delay(50);
        continue;
      }

      p = finger.image2Tz(1);
      if (p == FINGERPRINT_OK) {
        return true;
      }
      delay(50);
    }
    return false;
  }
  uint8_t addFinger(uint16_t id = 1, unsigned long timeout = 10000) {
    unsigned long startTime = millis();
    uint8_t p;

    while (millis() - startTime < timeout) {
      p = finger.getImage();
      if (p != FINGERPRINT_OK) {
        delay(50);
        continue;
      }

      p = finger.image2Tz(2);
      if (p == FINGERPRINT_OK) {
        break;
      }

      delay(50);
    }

    if (millis() - startTime >= timeout) {
      return FINGERPRINT_TIMEOUT;
    }

    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
      return p;
    }

    p = finger.storeModel(id);
    if (p != FINGERPRINT_OK) {
      return p;
    }

    return FINGERPRINT_OK;
  }
  boolean verifyPassword(){
    return finger.verifyPassword();
  }
};