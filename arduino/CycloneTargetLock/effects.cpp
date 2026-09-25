#include "effects.h"

const char* const STYLE_NAMES[STYLE_COUNT] = {
  "Blink", "Ripple", "Sparkle", "Rainbow", "Spin", "Fill up"
};

const char* const IDLE_NAMES[IDLE_COUNT] = {
  "Off", "Chase", "Rainbow", "Breathe", "Sparkle"
};

const char* const DIR_NAMES[DIR_COUNT] = {
  "Clockwise", "Counter-clockwise", "Flip on every hit", "Random on every hit"
};

void fxFrame(uint8_t style, CRGB color, uint32_t elapsed, uint16_t halfMs, int center, uint8_t n) {

  uint32_t period = 2UL * halfMs;

  uint32_t inCycle = elapsed % period;

  uint8_t phase = (inCycle * 255UL) / period;

  fill_solid(leds, MAX_LEDS, CRGB::Black);

  switch (style) {

    case 0:   // Blink: whole ring on, then off
      if (inCycle < halfMs)
        fill_solid(leds, n, color);
      break;

    case 1: { // Ripple: a ring expanding from the centre LED
      int radius = (phase * (n / 2 + 2)) / 255;

      for (int i = 0; i < n; i++) {

        int d = abs(i - center);

        if (d > n - d) d = n - d;

        if (d <= radius && d >= radius - 1)
          leds[i] = color;
      }
      break;
    }

    case 2:   // Sparkle: random twinkles
      for (int k = 0; k < n / 3 + 1; k++) {

        CRGB c = color;

        c.nscale8(random8(60, 255));

        leds[random(n)] = c;
      }
      break;

    case 3:   // Rainbow: rotating colour wheel
      fill_rainbow(leds, n, phase, 256 / n);
      break;

    case 4: { // Spin: a comet running round the ring
      int head = (phase * n) / 256;

      for (int t = 0; t < 6 && t < n; t++) {

        CRGB c = color;

        c.nscale8(255 - t * 40);

        leds[(head - t + n) % n] = c;
      }
      break;
    }

    case 5: { // Fill up: ring fills from the centre LED
      int count = (phase * n) / 255 + 1;

      for (int i = 0; i < count && i < n; i++)
        leds[(center + i) % n] = color;
      break;
    }
  }
}

void idleFrame(uint8_t mode, CRGB color, uint8_t stepMs, uint8_t n) {

  uint32_t now = millis();

  switch (mode) {

    case 1: { // Chase
      fill_solid(leds, MAX_LEDS, CRGB::Black);

      int head = (now / stepMs) % n;

      for (int t = 0; t < 5 && t < n; t++) {

        CRGB c = color;

        c.nscale8(255 - t * 50);

        leds[(head - t + n) % n] = c;
      }
      break;
    }

    case 2:   // Rainbow
      fill_rainbow(leds, n, (uint8_t)(now / 20), 256 / n);
      break;

    case 3: { // Breathe
      CRGB c = color;

      c.nscale8(beatsin8(12, 10, 255));

      fill_solid(leds, n, c);
      break;
    }

    case 4:   // Sparkle
      fadeToBlackBy(leds, n, 40);

      if (random8() < 90)
        leds[random(n)] = color;
      break;

    default:  // Off
      fill_solid(leds, MAX_LEDS, CRGB::Black);
      break;
  }
}

void sweepFrame(CRGB color, uint32_t elapsed, uint8_t perLedMs, uint8_t n) {

  fill_solid(leds, MAX_LEDS, CRGB::Black);

  if (perLedMs == 0)
    return;

  uint32_t count = elapsed / perLedMs;

  for (uint32_t i = 0; i < count && i < n; i++)
    leds[i] = color;
}
