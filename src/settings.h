// Cyclone Target Lock - all tunable settings, presets and EEPROM storage
#pragma once

#include <Arduino.h>

#define MAX_LEDS         24
#define PRESET_COUNT     7
#define SLOT_COUNT       3

#define SETTINGS_MAGIC   0xC7C1
#define SETTINGS_LAYOUT  3

// Everything the web page can change. Colours are 0xRRGGBB.
struct Settings {

  uint16_t magic;
  uint16_t layout;
  uint16_t size;

  // ---- game rules ----
  uint16_t roundSeconds;     // length of one round
  uint16_t speedDelay;       // ms per cursor step at score 0 (lower = faster)
  uint16_t minDelay;         // fastest the cursor may get
  uint16_t winScore;         // 0 = off, otherwise reaching it wins the round
  uint8_t  speedStep;        // ms faster per point
  uint8_t  pointsPerLevel;
  uint8_t  hitPoints;
  uint8_t  missPenalty;
  uint8_t  hitWindow;        // hit counts if cursor is within +/- this many LEDs
  uint8_t  timeBonus;        // seconds added on a hit
  uint8_t  timePenalty;      // seconds removed on a miss
  uint8_t  dirMode;          // 0 cw, 1 ccw, 2 flip on hit, 3 random on hit
  uint8_t  ledCount;
  uint8_t  brightness;

  // ---- colours ----
  uint32_t cTarget;
  uint32_t cBg;
  uint32_t cCursor;
  uint32_t cHit;
  uint32_t cMiss;
  uint32_t cOver;
  uint32_t cWin;
  uint32_t cStart;
  uint32_t cIdle;

  // ---- cursor look, idle animation, start sweep ----
  uint8_t  tailLen;
  uint8_t  targetPulse;
  uint8_t  idleMode;         // 0 off, 1 chase, 2 rainbow, 3 breathe, 4 sparkle
  uint8_t  idleSpeed;
  uint8_t  startSweep;       // ms per LED, 0 = no sweep

  // ---- effects: style, blink count, half-blink time ----
  uint8_t  hitStyle;   uint8_t hitBlinks;   uint16_t hitMs;
  uint8_t  missStyle;  uint8_t missBlinks;  uint16_t missMs;
  uint8_t  overStyle;  uint8_t overBlinks;  uint16_t overMs;
  uint8_t  winStyle;   uint8_t winBlinks;   uint16_t winMs;

  // ---- buzzer ----
  uint8_t  buzzer;
  uint8_t  sndStart;
  uint8_t  sndHit;
  uint8_t  sndLevel;
  uint8_t  sndMiss;
  uint8_t  sndOver;
  uint8_t  sndWin;
  uint8_t  sndTick;
  uint8_t  tickSec;          // tick during the last N seconds, 0 = off
  uint8_t  stepClick;        // tiny click on every cursor step
};

// Describes one numeric/colour field so the web API can read and write it by name.
struct Field {
  const char* key;
  void*       ptr;
  uint8_t     size;          // 1, 2 or 4 bytes
  uint32_t    minV;
  uint32_t    maxV;
  bool        isColor;
};

#define CFG_ADDR    16
#define EEPROM_SIZE (CFG_ADDR + (SLOT_COUNT + 1) * sizeof(Settings))

extern Settings cfg;

void settingsBegin();                       // EEPROM.begin + load (or defaults)
void settingsDefaults(Settings& s);
void settingsClamp();
void settingsSave();
void settingsApplyPreset(uint8_t id);       // keeps ledCount
const char* presetName(uint8_t id);
const char* presetDesc(uint8_t id);

bool settingsSetField(const String& key, const String& value);
String settingsValuesJson();
String settingsLimitsJson();

bool slotUsed(uint8_t i);
bool slotSave(uint8_t i);
bool slotLoad(uint8_t i);

int  highScoreLoad();
void highScoreSave(int value);
