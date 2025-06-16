#include <LiquidCrystal.h>

class LCD {
private:
  LiquidCrystal lcd;

public:
  LCD():lcd(A0,A1,A2,A3,A4,A5){};

  void begin() {
    lcd.begin(16, 2);
    lcd.home();
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

  void display(String row1, String row2, byte delayTime) {
    lcd.clear();
    lcd.print(row1);
    lcd.setCursor(0, 1);
    lcd.print(row2);
    delay(delayTime * 1000);
  }
};
