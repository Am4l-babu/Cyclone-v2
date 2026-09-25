/*
   ============================================================
     BUTTON TEST  -  Cyclone Target Lock
   ============================================================

   Checks that the push button is wired correctly and debounces
   cleanly. Open the Serial Monitor at 115200 baud.

   WIRING
   ------------------------------------------------------------
   BUTTON leg 1  -> D5 / GPIO14
   BUTTON leg 2  -> GND
   (internal pull-up is used, no resistor needed)

   EXPECTED RESULT
   ------------------------------------------------------------
   - Idle:            pin reads HIGH
   - Pressed:         "PRESS #n" is printed, on-board LED lights
   - Released:        "RELEASE" + how long you held it
   - One press = one message. If you see several messages per
     press the button is bouncing badly or the wiring is loose.
*/

#include <Arduino.h>

#define BUTTON_PIN     D5
#define DEBOUNCE_MS    40

bool lastReading = HIGH;
bool stableState = HIGH;

unsigned long lastChangeTime = 0;
unsigned long pressStart     = 0;
unsigned long pressCount     = 0;
unsigned long lastReport     = 0;

void setup() {

  Serial.begin(115200);

  delay(500);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // On-board LED is active LOW on NodeMCU
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println();
  Serial.println("==============================");
  Serial.println(" BUTTON TEST  (pin D5 / GPIO14)");
  Serial.println("==============================");
  Serial.println("Press the button...");
}

void loop() {

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastChangeTime = millis();
  }

  if (millis() - lastChangeTime > DEBOUNCE_MS) {

    if (reading != stableState) {

      stableState = reading;

      if (stableState == LOW) {

        pressCount++;
        pressStart = millis();

        digitalWrite(LED_BUILTIN, LOW);

        Serial.print("PRESS   #");
        Serial.println(pressCount);

      } else {

        digitalWrite(LED_BUILTIN, HIGH);

        Serial.print("RELEASE  held for ");
        Serial.print(millis() - pressStart);
        Serial.println(" ms");
      }
    }
  }

  lastReading = reading;

  // Heartbeat so you know the sketch is alive
  if (millis() - lastReport > 5000) {

    lastReport = millis();

    Serial.print("[idle] raw pin = ");
    Serial.print(reading ? "HIGH (not pressed)" : "LOW (pressed)");
    Serial.print(" | presses so far: ");
    Serial.println(pressCount);
  }
}
