#include "sounds.h"
#include "settings.h"

struct Note {
  uint16_t freq;   // Hz, 0 = rest
  uint16_t ms;
};

struct Sound {
  const char* name;
  const Note* notes;
  uint8_t     len;
};

#define SEQ(arr) arr, (uint8_t)(sizeof(arr) / sizeof(arr[0]))

static const Note N_BEEP[]     = { {1200, 80} };
static const Note N_CHIRP_UP[] = { {800, 40}, {1100, 40}, {1500, 50} };
static const Note N_CHIRP_DN[] = { {1400, 45}, {1000, 45}, {650, 60} };
static const Note N_COIN[]     = { {988, 70}, {1319, 220} };
static const Note N_BUZZ[]     = { {160, 120}, {0, 30}, {160, 120} };
static const Note N_FANFARE[]  = { {523, 110}, {0, 25}, {523, 110}, {0, 25}, {523, 110},
                                   {0, 25}, {659, 180}, {784, 140}, {1047, 450} };
static const Note N_SAD[]      = { {392, 250}, {370, 250}, {349, 250}, {330, 600} };
static const Note N_SIREN[]    = { {900, 150}, {1200, 150}, {900, 150},
                                   {1200, 150}, {900, 150}, {1200, 150} };
static const Note N_POWERUP[]  = { {523, 60}, {659, 60}, {784, 60}, {1047, 60}, {1319, 60}, {1568, 160} };
static const Note N_LASER[]    = { {2400, 15}, {2000, 15}, {1600, 15}, {1300, 15},
                                   {1000, 15}, {800, 15}, {600, 15}, {450, 20} };
static const Note N_TICK[]     = { {2200, 25} };

// Index in this table == sound id used by the settings / web page.
static const Sound SOUNDS[SOUND_COUNT] = {
  { "Off",          nullptr, 0 },
  { "Beep",         SEQ(N_BEEP) },
  { "Chirp up",     SEQ(N_CHIRP_UP) },
  { "Chirp down",   SEQ(N_CHIRP_DN) },
  { "Coin",         SEQ(N_COIN) },
  { "Low buzz",     SEQ(N_BUZZ) },
  { "Fanfare",      SEQ(N_FANFARE) },
  { "Sad trombone", SEQ(N_SAD) },
  { "Siren",        SEQ(N_SIREN) },
  { "Power up",     SEQ(N_POWERUP) },
  { "Laser",        SEQ(N_LASER) },
  { "Tick",         SEQ(N_TICK) },
};

const char* soundName(uint8_t id) {

  return id < SOUND_COUNT ? SOUNDS[id].name : "?";
}

static const Note* seq       = nullptr;
static uint8_t     seqLen    = 0;
static uint8_t     seqIndex  = 0;
static bool        active    = false;
static uint32_t    nextNoteAt = 0;

void soundBegin() {

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
}

void soundStop() {

  active = false;

  noTone(BUZZER_PIN);

  digitalWrite(BUZZER_PIN, LOW);
}

bool soundBusy() {

  return active;
}

void soundPlay(uint8_t id, bool force) {

  if (!force && !cfg.buzzer)
    return;

  if (id == 0 || id >= SOUND_COUNT)
    return;

  seq      = SOUNDS[id].notes;
  seqLen   = SOUNDS[id].len;
  seqIndex = 0;
  active   = true;

  nextNoteAt = millis();
}

void soundLoop() {

  if (!active)
    return;

  if ((int32_t)(millis() - nextNoteAt) < 0)
    return;

  if (seqIndex >= seqLen) {

    soundStop();

    return;
  }

  const Note& n = seq[seqIndex++];

  if (n.freq)
    tone(BUZZER_PIN, n.freq);
  else
    noTone(BUZZER_PIN);

  nextNoteAt = millis() + n.ms;
}
