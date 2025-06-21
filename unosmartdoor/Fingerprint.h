#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

class Fingerprint {
private:
  HardwareSerial mySerial;
  Adafruit_Fingerprint finger;
  boolean firstImageSet = false;

public:
  Fingerprint()
    : mySerial(2), finger(&mySerial) {}

  void begin() {
    mySerial.begin(57600, SERIAL_8N1, 16, 17);
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

  uint8_t addFinger() {

    uint8_t p;

    p = finger.getImage();
    if (p != FINGERPRINT_OK) {
      return p;
    }

    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
      return p;
    }

    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
      return p;
    }

    finger.getTemplateCount();
    p = finger.storeModel(finger.templateCount + 1);
    if (p != FINGERPRINT_OK) {
      return p;
    }
    
    return p;
  }

  boolean verifyPassword() {
    return finger.verifyPassword();
  }

  boolean hadFirstImage() {
    return firstImageSet;
  }

  void resetHadFirstImage(){
    firstImageSet = false;
  }

  boolean isFullData() {
    finger.getTemplateCount();
    return finger.templateCount == 127;
  }

  void emptyDatabase() {
    finger.emptyDatabase();
  }
};