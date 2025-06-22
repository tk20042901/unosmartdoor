#include <LiquidCrystal_I2C.h>

class LCD {
private:
  LiquidCrystal_I2C lcd;

public:
  LCD()
    : lcd(0x27, 16, 2){};

  void begin() {
    lcd.init();
    lcd.backlight();
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

  void display(String s) {
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
