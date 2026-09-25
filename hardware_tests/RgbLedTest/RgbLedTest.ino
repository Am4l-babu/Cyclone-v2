/*
   ============================================================
     RGB LED TEST  -  Cyclone Target Lock  (24 x WS2812B)
   ============================================================

   Runs a loop of visual patterns so you can spot dead LEDs,
   swapped colour order, wrong LED count or a weak power supply.
   Open the Serial Monitor at 115200 baud to see which stage
   is running.

   WIRING
   ------------------------------------------------------------
   WS2812B DIN   -> D6 / GPIO12   (a 330 ohm resistor in series is nice to have)
   WS2812B 5V    -> 5V            (external 5V supply recommended)
   WS2812B GND   -> GND           (must be common with the NodeMCU GND)
   Tip: a 1000 uF capacitor across 5V/GND at the ring helps.

   STAGES
   ------------------------------------------------------------
   1. All LEDs RED, then GREEN, then BLUE   -> checks colour order
      (if red shows as green, change GRB to RGB below)
   2. Single white dot walking round the ring -> checks every LED
   3. Cyclone chase (green cursor + red target, like the game)
   4. Rainbow wheel
   5. Brightness breathing
*/

#include <Arduino.h>
#include <FastLED.h>

#define LED_PIN      D6
#define NUM_LEDS     24
#define BRIGHTNESS   60          // keep low when powered from USB

// Change to RGB if the colours in stage 1 come out wrong
#define COLOR_ORDER  GRB

CRGB leds[NUM_LEDS];

void showAll(const CRGB& color, const char* name, uint16_t ms) {

  Serial.print("  all ");
  Serial.println(name);

  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();

  delay(ms);
}

void stageColors() {

  Serial.println("[1/5] Colour check");

  showAll(CRGB::Red,   "RED",   900);
  showAll(CRGB::Green, "GREEN", 900);
  showAll(CRGB::Blue,  "BLUE",  900);
  showAll(CRGB::White, "WHITE", 900);

  FastLED.clear();
  FastLED.show();
}

void stageWalk() {

  Serial.println("[2/5] Walking dot (watch for dead LEDs)");

  for (int i = 0; i < NUM_LEDS; i++) {

    FastLED.clear();
    leds[i] = CRGB::White;
    FastLED.show();

    Serial.print("  LED ");
    Serial.println(i + 1);

    delay(120);
  }

  FastLED.clear();
  FastLED.show();
}

void stageChase() {

  Serial.println("[3/5] Cyclone chase");

  int target = random(NUM_LEDS);

  for (int lap = 0; lap < 3; lap++) {

    for (int i = 0; i < NUM_LEDS; i++) {

      fill_solid(leds, NUM_LEDS, CRGB(0, 0, 20));
      leds[target] = CRGB::Red;
      leds[i]      = CRGB::Green;
      FastLED.show();

      delay(45);
    }

    target = random(NUM_LEDS);
  }

  FastLED.clear();
  FastLED.show();
}

void stageRainbow() {

  Serial.println("[4/5] Rainbow");

  uint8_t hue = 0;

  for (int frame = 0; frame < 250; frame++) {

    fill_rainbow(leds, NUM_LEDS, hue, 256 / NUM_LEDS);
    FastLED.show();

    hue += 3;

    delay(15);
  }

  FastLED.clear();
  FastLED.show();
}

void stageBreathe() {

  Serial.println("[5/5] Brightness breathing");

  fill_solid(leds, NUM_LEDS, CRGB::Cyan);

  for (int pass = 0; pass < 2; pass++) {

    for (int b = 5; b <= 150; b += 3) {
      FastLED.setBrightness(b);
      FastLED.show();
      delay(10);
    }

    for (int b = 150; b >= 5; b -= 3) {
      FastLED.setBrightness(b);
      FastLED.show();
      delay(10);
    }
  }

  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

void setup() {

  Serial.begin(115200);

  delay(500);

  randomSeed(micros());

  FastLED.addLeds<WS2812B, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  Serial.println();
  Serial.println("==============================");
  Serial.println(" RGB LED TEST  (24 x WS2812B, D6)");
  Serial.println("==============================");
}

void loop() {

  stageColors();
  stageWalk();
  stageChase();
  stageRainbow();
  stageBreathe();

  Serial.println("--- loop complete, restarting ---");
  delay(500);
}
