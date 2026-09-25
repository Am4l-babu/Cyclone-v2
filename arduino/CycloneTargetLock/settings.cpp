#include "settings.h"
#include "sounds.h"

#include <EEPROM.h>

Settings cfg;

// ============================================================
// FIELD TABLE  (name used by the web page  ->  variable + limits)
// ============================================================

#define N8(k, mn, mx)   { #k, &cfg.k, 1, mn, mx, false }
#define N16(k, mn, mx)  { #k, &cfg.k, 2, mn, mx, false }
#define CLR(k)          { #k, &cfg.k, 4, 0, 0xFFFFFF, true }

static const Field FIELDS[] = {

  // game rules
  N16(roundSeconds, 10, 600),
  N16(speedDelay, 10, 1000),
  N16(minDelay, 5, 500),
  N16(winScore, 0, 999),
  N8(speedStep, 0, 20),
  N8(pointsPerLevel, 1, 50),
  N8(hitPoints, 1, 10),
  N8(missPenalty, 0, 10),
  N8(hitWindow, 0, 3),
  N8(timeBonus, 0, 10),
  N8(timePenalty, 0, 10),
  N8(dirMode, 0, 3),
  N8(ledCount, 4, MAX_LEDS),
  N8(brightness, 1, 255),

  // colours
  CLR(cTarget), CLR(cBg), CLR(cCursor), CLR(cHit), CLR(cMiss),
  CLR(cOver), CLR(cWin), CLR(cStart), CLR(cIdle),

  // look
  N8(tailLen, 0, 8),
  N8(targetPulse, 0, 1),
  N8(idleMode, 0, 4),
  N8(idleSpeed, 10, 250),
  N8(startSweep, 0, 100),

  // effects
  N8(hitStyle, 0, 5),  N8(hitBlinks, 0, 10),  N16(hitMs, 20, 500),
  N8(missStyle, 0, 5), N8(missBlinks, 0, 10), N16(missMs, 20, 500),
  N8(overStyle, 0, 5), N8(overBlinks, 0, 20), N16(overMs, 20, 1000),
  N8(winStyle, 0, 5),  N8(winBlinks, 0, 20),  N16(winMs, 20, 1000),

  // buzzer
  N8(buzzer, 0, 1),
  N8(sndStart, 0, SOUND_COUNT - 1),
  N8(sndHit, 0, SOUND_COUNT - 1),
  N8(sndLevel, 0, SOUND_COUNT - 1),
  N8(sndMiss, 0, SOUND_COUNT - 1),
  N8(sndOver, 0, SOUND_COUNT - 1),
  N8(sndWin, 0, SOUND_COUNT - 1),
  N8(sndTick, 0, SOUND_COUNT - 1),
  N8(tickSec, 0, 30),
  N8(stepClick, 0, 1),
};

static const size_t FIELD_COUNT = sizeof(FIELDS) / sizeof(FIELDS[0]);

static uint32_t readField(const Field& f) {

  uint32_t v = 0;

  memcpy(&v, f.ptr, f.size);

  return v;
}

static void writeField(const Field& f, uint32_t v) {

  memcpy(f.ptr, &v, f.size);
}

void settingsClamp() {

  for (size_t i = 0; i < FIELD_COUNT; i++) {

    uint32_t v = readField(FIELDS[i]);

    if (v < FIELDS[i].minV) v = FIELDS[i].minV;

    if (v > FIELDS[i].maxV) v = FIELDS[i].maxV;

    writeField(FIELDS[i], v);
  }
}

bool settingsSetField(const String& key, const String& value) {

  for (size_t i = 0; i < FIELD_COUNT; i++) {

    if (key != FIELDS[i].key)
      continue;

    const Field& f = FIELDS[i];

    long v;

    if (f.isColor) {

      String hex = value;

      if (hex.startsWith("#"))
        hex.remove(0, 1);

      v = strtoul(hex.c_str(), NULL, 16);

    } else {

      v = value.toInt();

      if (v < 0)
        v = 0;
    }

    if ((uint32_t)v < f.minV) v = f.minV;

    if ((uint32_t)v > f.maxV) v = f.maxV;

    writeField(f, (uint32_t)v);

    return true;
  }

  return false;
}

String settingsValuesJson() {

  String j;

  j.reserve(1500);

  j += '{';

  for (size_t i = 0; i < FIELD_COUNT; i++) {

    if (i) j += ',';

    j += '"';
    j += FIELDS[i].key;
    j += "\":";

    uint32_t v = readField(FIELDS[i]);

    if (FIELDS[i].isColor) {

      char buf[12];

      snprintf(buf, sizeof(buf), "\"#%06X\"", (unsigned)v);

      j += buf;

    } else {

      j += v;
    }
  }

  j += '}';

  return j;
}

String settingsLimitsJson() {

  String j;

  j.reserve(1200);

  j += '{';

  bool first = true;

  for (size_t i = 0; i < FIELD_COUNT; i++) {

    if (FIELDS[i].isColor)
      continue;

    if (!first) j += ',';

    first = false;

    j += '"';
    j += FIELDS[i].key;
    j += "\":[";
    j += FIELDS[i].minV;
    j += ',';
    j += FIELDS[i].maxV;
    j += ']';
  }

  j += '}';

  return j;
}

// ============================================================
// DEFAULTS + PRESETS
// ============================================================

void settingsDefaults(Settings& s) {

  memset(&s, 0, sizeof(s));

  s.magic  = SETTINGS_MAGIC;
  s.layout = SETTINGS_LAYOUT;
  s.size   = sizeof(Settings);

  // game rules
  s.roundSeconds   = 60;
  s.speedDelay     = 80;
  s.minDelay       = 20;
  s.winScore       = 0;
  s.speedStep      = 2;
  s.pointsPerLevel = 5;
  s.hitPoints      = 1;
  s.missPenalty    = 1;
  s.hitWindow      = 0;
  s.timeBonus      = 0;
  s.timePenalty    = 0;
  s.dirMode        = 0;
  s.ledCount       = 24;
  s.brightness     = 100;

  // colours
  s.cTarget = 0xFF0000;
  s.cBg     = 0x000014;
  s.cCursor = 0x00FF00;
  s.cHit    = 0x00FF00;
  s.cMiss   = 0xFF0000;
  s.cOver   = 0xFF0000;
  s.cWin    = 0xFFD700;
  s.cStart  = 0x0000FF;
  s.cIdle   = 0x00AFFF;

  // look
  s.tailLen     = 0;
  s.targetPulse = 0;
  s.idleMode    = 1;
  s.idleSpeed   = 70;
  s.startSweep  = 25;

  // effects
  s.hitStyle  = 0; s.hitBlinks  = 1; s.hitMs  = 40;
  s.missStyle = 0; s.missBlinks = 1; s.missMs = 60;
  s.overStyle = 0; s.overBlinks = 3; s.overMs = 150;
  s.winStyle  = 3; s.winBlinks  = 3; s.winMs  = 250;

  // buzzer
  s.buzzer    = 1;
  s.sndStart  = 9;    // power up
  s.sndHit    = 2;    // chirp up
  s.sndLevel  = 4;    // coin
  s.sndMiss   = 5;    // low buzz
  s.sndOver   = 7;    // sad trombone
  s.sndWin    = 6;    // fanfare
  s.sndTick   = 11;   // tick
  s.tickSec   = 5;
  s.stepClick = 0;
}

static const char* const PRESET_NAMES[PRESET_COUNT] = {
  "Classic", "Neon Night", "Arcade", "Easy", "Hard", "Party", "Stealth"
};

static const char* const PRESET_DESCS[PRESET_COUNT] = {
  "The original look: green cursor, red target, 60 s round.",
  "Cyan comet, pulsing magenta target, ripple effects.",
  "Yellow cursor, coin sounds, fill-up hit effect.",
  "Slower cursor, wider hit window, no penalty, 90 s.",
  "Fast cursor, misses cost points and time, 45 s.",
  "Rainbow idle, sparkles, random direction, big finale.",
  "Dim and silent. Perfect for late-night play."
};

const char* presetName(uint8_t id) { return id < PRESET_COUNT ? PRESET_NAMES[id] : "?"; }
const char* presetDesc(uint8_t id) { return id < PRESET_COUNT ? PRESET_DESCS[id] : "";  }

void settingsApplyPreset(uint8_t id) {

  uint8_t keepLeds = cfg.ledCount;

  settingsDefaults(cfg);

  cfg.ledCount = keepLeds;

  switch (id) {

    case 1:   // Neon Night
      cfg.cTarget = 0xFF00C8; cfg.cBg = 0x0A001E; cfg.cCursor = 0x00FFFF;
      cfg.cHit = 0x00FFFF;    cfg.cMiss = 0xFF0050; cfg.cOver = 0xFF0050;
      cfg.cWin = 0xFF00C8;    cfg.cIdle = 0x00FFFF;
      cfg.tailLen = 4;        cfg.targetPulse = 1;
      cfg.hitStyle = 1;       cfg.hitBlinks = 1;  cfg.hitMs = 120;
      cfg.overStyle = 4;      cfg.overBlinks = 3; cfg.overMs = 250;
      cfg.winStyle = 1;       cfg.winBlinks = 4;  cfg.winMs = 200;
      cfg.sndHit = 10;
      break;

    case 2:   // Arcade
      cfg.cCursor = 0xFFC800; cfg.cTarget = 0xFF2020; cfg.cBg = 0x100800;
      cfg.cHit = 0xFFC800;    cfg.cIdle = 0xFFC800;
      cfg.tailLen = 2;
      cfg.hitStyle = 5;       cfg.hitBlinks = 1;  cfg.hitMs = 70;
      cfg.sndHit = 4;         cfg.sndLevel = 9;
      break;

    case 3:   // Easy
      cfg.speedDelay = 140;   cfg.minDelay = 60;  cfg.speedStep = 1;
      cfg.hitWindow = 1;      cfg.missPenalty = 0;
      cfg.roundSeconds = 90;  cfg.timeBonus = 1;
      cfg.tailLen = 3;        cfg.brightness = 130;
      break;

    case 4:   // Hard
      cfg.speedDelay = 55;    cfg.minDelay = 12;  cfg.speedStep = 3;
      cfg.hitWindow = 0;      cfg.missPenalty = 2; cfg.timePenalty = 2;
      cfg.roundSeconds = 45;  cfg.pointsPerLevel = 3;
      cfg.dirMode = 2;
      cfg.tickSec = 10;
      break;

    case 5:   // Party
      cfg.idleMode = 2;       cfg.cBg = 0x000000;
      cfg.tailLen = 5;        cfg.targetPulse = 1;
      cfg.dirMode = 3;        cfg.brightness = 160;
      cfg.hitStyle = 2;       cfg.hitBlinks = 2;  cfg.hitMs = 60;
      cfg.overStyle = 1;      cfg.overBlinks = 4; cfg.overMs = 200;
      cfg.winStyle = 3;       cfg.winBlinks = 8;  cfg.winMs = 200;
      break;

    case 6:   // Stealth
      cfg.brightness = 30;    cfg.buzzer = 0;     cfg.idleMode = 0;
      cfg.cBg = 0x000000;     cfg.cCursor = 0x00A000; cfg.cTarget = 0xA00000;
      cfg.hitBlinks = 0;      cfg.missBlinks = 0;
      break;

    default:  // 0 = Classic, plain defaults
      break;
  }

  settingsClamp();
}

// ============================================================
// EEPROM
// ============================================================

static bool valid(const Settings& s) {

  return s.magic == SETTINGS_MAGIC &&
         s.layout == SETTINGS_LAYOUT &&
         s.size == sizeof(Settings);
}

static int slotAddr(uint8_t i) {

  return CFG_ADDR + (i + 1) * sizeof(Settings);
}

void settingsBegin() {

  EEPROM.begin(EEPROM_SIZE);

  Settings tmp;

  EEPROM.get(CFG_ADDR, tmp);

  if (valid(tmp)) {

    cfg = tmp;

    settingsClamp();

  } else {

    settingsDefaults(cfg);

    settingsSave();
  }
}

void settingsSave() {

  EEPROM.put(CFG_ADDR, cfg);

  EEPROM.commit();
}

bool slotUsed(uint8_t i) {

  if (i >= SLOT_COUNT)
    return false;

  Settings tmp;

  EEPROM.get(slotAddr(i), tmp);

  return valid(tmp);
}

bool slotSave(uint8_t i) {

  if (i >= SLOT_COUNT)
    return false;

  EEPROM.put(slotAddr(i), cfg);

  EEPROM.commit();

  return true;
}

bool slotLoad(uint8_t i) {

  if (i >= SLOT_COUNT)
    return false;

  Settings tmp;

  EEPROM.get(slotAddr(i), tmp);

  if (!valid(tmp))
    return false;

  cfg = tmp;

  settingsClamp();

  return true;
}

int highScoreLoad() {

  int v;

  EEPROM.get(0, v);

  if (v < 0 || v > 9999)
    v = 0;

  return v;
}

void highScoreSave(int value) {

  EEPROM.put(0, value);

  EEPROM.commit();
}
