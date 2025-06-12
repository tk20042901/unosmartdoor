#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
class Fingerprint
{
private:
    SoftwareSerial mySerial;
    Adafruit_Fingerprint finger;
    boolean isFirstTemplate;

public:
    Fingerprint(byte inPin, byte outPin)
        : mySerial(inPin, outPin), finger(&mySerial), isFirstTemplate(false) {}

    void begin()
    {
        finger.begin(57600);
    }
    char checkFingerprint()
    {
        uint8_t p = finger.getImage();
        if (p != FINGERPRINT_OK)
            return p;

        p = finger.image2Tz();
        if (p != FINGERPRINT_OK)
            return p;

        p = finger.fingerFastSearch();

        return p;
    }
    char setFirstTemplate()
    {
        if (isFirstTemplate)
        {
            return FINGERPRINT_OK;
        }
        uint8_t p = finger.getImage();
        if (p != FINGERPRINT_OK)
            return p;
        p = finger.image2Tz(1);
        if (p == FINGERPRINT_OK)
        {
            isFirstTemplate = true;
            delay(2000);
        }
        return p;
    }
    char createNewFingerprintModel()
    {
        uint8_t p = finger.getImage();
        if (p != FINGERPRINT_OK)
            return p;
        p = finger.image2Tz(2);
        if (p != FINGERPRINT_OK)
            return p;
        finger.createModel();
        if (p == FINGERPRINT_OK)
        {
            isFirstTemplate = false;
            finger.storeModel(1);
        }
        return p;
    }
    void deleteFirstTemplate()
    {
        isFirstTemplate = false;
    }
    int getFingerprintIDez()
    {
        uint8_t p = finger.getImage();
        if (p != FINGERPRINT_OK)
            return -1;

        p = finger.image2Tz();
        if (p != FINGERPRINT_OK)
            return -1;

        p = finger.fingerFastSearch();
        if (p != FINGERPRINT_OK)
            return -1;
        return finger.fingerID;
    }
    
    int getFirstImage(){
        uint8_t p = finger.getImage();
        if (p != FINGERPRINT_OK)
            return false;
        p = finger.image2Tz(1);
        if (p != FINGERPRINT_OK)
            return -1;
    }
    int addFinger(){
        uint8_t p = finger.getImage();
        if (p != FINGERPRINT_OK)
            return false;
        p = 0;
        p = finger.image2Tz(2);
        if (p != FINGERPRINT_OK)
            return -1;
        p = finger.createModel();
        if (p != FINGERPRINT_OK)
            return -1;
        p = finger.storeModel(1);
        if (p != FINGERPRINT_OK)
            return -1;
        return true;
    }
    boolean isFirstImage(){
        return isFirstTemplate;
    }
};