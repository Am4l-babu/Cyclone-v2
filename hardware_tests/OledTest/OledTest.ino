/*
   ============================================================
     OLED TEST  -  Cyclone Target Lock  (SSD1306 128x64 I2C)
   ============================================================

   Scans the I2C bus, initialises the display and cycles through
   text, shapes, a progress bar and an inverted screen.
   Open the Serial Monitor at 115200 baud.

   WIRING
   ------------------------------------------------------------
   OLED SDA -> D2 / GPIO4
   OLED SCL -> D1 / GPIO5
   OLED VCC -> 3.3V
   OLED GND -> GND

   TROUBLESHOOTING
   ------------------------------------------------------------
   - "No I2C devices found"  -> check SDA/SCL are not swapped,
                                check 3.3V and GND.
   - Device found at 0x3D    -> change OLED_ADDRESS below to 0x3D.
   - Display found but blank -> some modules are SH1106, not SSD1306.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_SDA      D2
#define OLED_SCL      D1

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool oledReady = false;

void scanI2C() {

  Serial.println("Scanning I2C bus...");

  int found = 0;

  for (uint8_t address = 1; address < 127; address++) {

    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0) {

      Serial.print("  device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);

      found++;
    }
  }

  if (found == 0) {
    Serial.println("  No I2C devices found - check wiring!");
  }
}

void stageText() {

  Serial.println("[1/5] Text sizes");

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Size 1: OLED works!");

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.println("Size 2");

  display.setTextSize(3);
  display.setCursor(0, 38);
  display.println("Size3");

  display.display();

  delay(2000);
}

void stageShapes() {

  Serial.println("[2/5] Shapes");

  display.clearDisplay();

  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawCircle(32, 32, 20, SSD1306_WHITE);
  display.fillCircle(32, 32, 8, SSD1306_WHITE);
  display.fillRect(70, 12, 40, 16, SSD1306_WHITE);
  display.drawTriangle(70, 56, 90, 34, 110, 56, SSD1306_WHITE);

  display.display();

  delay(2000);
}

void stageRing() {

  Serial.println("[3/5] Spinning ring (like the LED ring)");

  const int cx = 64;
  const int cy = 32;
  const int r  = 26;

  for (int frame = 0; frame < 48; frame++) {

    display.clearDisplay();

    for (int i = 0; i < 24; i++) {

      float a = (i * 15) * DEG_TO_RAD;

      int x = cx + cos(a) * r;
      int y = cy + sin(a) * r;

      if (i == frame % 24) {
        display.fillCircle(x, y, 3, SSD1306_WHITE);   // cursor
      } else if (i == 6) {
        display.drawCircle(x, y, 3, SSD1306_WHITE);   // target
      } else {
        display.drawPixel(x, y, SSD1306_WHITE);
      }
    }

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(40, 28);
    display.print("TARGET");

    display.display();

    delay(60);
  }
}

void stageProgress() {

  Serial.println("[4/5] Progress bar + counter");

  for (int p = 0; p <= 100; p += 4) {

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Loading...");

    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(p);
    display.print("%");

    display.drawRect(0, 44, 128, 14, SSD1306_WHITE);
    display.fillRect(2, 46, map(p, 0, 100, 0, 124), 10, SSD1306_WHITE);

    display.display();

    delay(40);
  }

  delay(500);
}

void stageInvert() {

  Serial.println("[5/5] Invert display");

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 8);
  display.println("INVERT");
  display.setTextSize(1);
  display.setCursor(10, 40);
  display.println("Screen flashing...");
  display.display();

  for (int i = 0; i < 4; i++) {

    display.invertDisplay(true);
    delay(400);

    display.invertDisplay(false);
    delay(400);
  }
}

void setup() {

  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("==============================");
  Serial.println(" OLED TEST  (SSD1306 128x64)");
  Serial.println("==============================");

  Wire.begin(OLED_SDA, OLED_SCL);

  scanI2C();

  oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);

  if (oledReady) {
    Serial.println("Display initialised OK");
  } else {
    Serial.println("Display init FAILED (see troubleshooting in this file)");
  }
}

void loop() {

  if (!oledReady) {

    // Keep retrying so you can fix the wiring without re-flashing
    delay(2000);

    Serial.println("Retrying display init...");

    oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);

    return;
  }

  stageText();
  stageShapes();
  stageRing();
  stageProgress();
  stageInvert();

  Serial.println("--- loop complete, restarting ---");
}
