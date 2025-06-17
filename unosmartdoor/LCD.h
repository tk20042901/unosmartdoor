#include <LiquidCrystal.h>

class LCD {
private:
  LiquidCrystal lcd;
  byte SDA_pin, SCL_pin;

public:
  LCD(byte SDA_pin, byte SCL_pin)
    : lcd(13, 12, 14, 27, 26, 25), SDA_pin(SDA_pin), SCL_pin(SCL_pin){};

  void begin() {
    lcd.begin(16, 2);
  }

  void clear() {
    lcd.clear();
  }

  void print(String s) {
    lcd.print(s);
  }

  void print(byte n) {
    lcd.print(n);
  }

  void setCursor(byte col, byte row) {
    lcd.setCursor(col, row);
  }

  void display(const String &s) {
    lcd.clear();
    lcd.print(s);
  }

  void display(String row1, String row2) {
    lcd.clear();
    lcd.print(row1);
    lcd.setCursor(0, 1);
    lcd.print(row2);
  }

   void display(String row1, byte delayTime) {
    lcd.clear();
    lcd.print(row1);
    delay(delayTime * 1000);
  }

  void display(String row1, String row2, byte delayTime) {
    lcd.clear();
    lcd.print(row1);
    lcd.setCursor(0, 1);
    lcd.print(row2);
    delay(delayTime * 1000);
  }
};
