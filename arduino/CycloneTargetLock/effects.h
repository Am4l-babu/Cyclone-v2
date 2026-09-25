// Cyclone Target Lock - LED effect renderers (pure frame generators, no delays)
#pragma once

#include <Arduino.h>
#include <FastLED.h>

#include "settings.h"

#define STYLE_COUNT 6
#define IDLE_COUNT  5
#define DIR_COUNT   4

extern const char* const STYLE_NAMES[STYLE_COUNT];
extern const char* const IDLE_NAMES[IDLE_COUNT];
extern const char* const DIR_NAMES[DIR_COUNT];

extern CRGB leds[MAX_LEDS];

// Render one frame of a celebration / feedback effect into leds[].
//   elapsed : ms since the effect began
//   halfMs  : one "blink" = 2 * halfMs
//   center  : LED where ripples / fills start (the cursor position)
void fxFrame(uint8_t style, CRGB color, uint32_t elapsed, uint16_t halfMs, int center, uint8_t n);

// One frame of the idle (attract mode) animation.
void idleFrame(uint8_t mode, CRGB color, uint8_t stepMs, uint8_t n);

// Start-of-round sweep: lights one more LED every perLedMs.
void sweepFrame(CRGB color, uint32_t elapsed, uint8_t perLedMs, uint8_t n);
