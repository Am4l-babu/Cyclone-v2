/*
   ============================================================
        CYCLONE TARGET LOCK  v2  -  ESP8266 LED reaction game
   ============================================================

   A cursor spins around a ring of WS2812B LEDs.
   Press the button the instant it lands on the target LED.
   Hit = points (and the cursor speeds up), miss = penalty.
   A round lasts 60 s by default, then the game ends.
   Everything (round time, colours, effects, sounds, presets,
   players, WiFi) is adjustable from the web page.

   Made by Am4l-babu  -  https://github.com/Am4l-babu

   PIN CONNECTIONS
   ------------------------------------------------------------
   WS2812B DATA  -> D6 / GPIO12
   BUTTON        -> D5 / GPIO14  (other leg to GND)
   BUTTON P2     -> D3 / GPIO0   (optional, duel mode, other leg to GND)
   BUZZER (+)    -> D7 / GPIO13  (other leg to GND, passive buzzer)

   OLED SDA      -> D2 / GPIO4
   OLED SCL      -> D1 / GPIO5
   OLED VCC      -> 3.3V
   OLED GND      -> GND

   WiFi AP:  SSID CycloneGame-XXXX (unique per board)  /  PASS 12345678
   Web:      joins open the page automatically (captive portal),
             or http://192.168.4.1  /  http://cyclone.local

   Hold the button while powering on (3 s) to clear the admin PIN,
   WiFi password and home WiFi settings.

   FILES: main.cpp (this file) + settings, sounds, effects, webui.
   For Arduino IDE run tools/sync_arduino.py (see README).
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <WebSocketsServer.h>

#include <FastLED.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "settings.h"
#include "sounds.h"
#include "effects.h"
#include "webui.h"

// ============================================================
// PIN DEFINITIONS + HARDWARE
// ============================================================

#define LED_PIN       D6
#define BUTTON_PIN    D5
#define BUTTON2_PIN   D3        // GPIO0: do not hold it while powering on

#define OLED_SDA      D2
#define OLED_SCL      D1

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

// FastLED power limiter (5 V, mA). Raise it if you use a big 5 V supply.
#define LED_MAX_MA    1500

// A press is taken on the first edge, then the button is ignored this long
// so contact bounce cannot count twice.
#define BUTTON_LOCK_MS  50

#define COMBO_MAX       4

// ============================================================
// OBJECTS
// ============================================================

CRGB leds[MAX_LEDS];

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

ESP8266WebServer        server(80);
ESP8266HTTPUpdateServer httpUpdater;
WebSocketsServer        ws(81);
DNSServer               dns;

char apSsid[24];

// ============================================================
// GAME STATE
// ============================================================

enum Mode : uint8_t {
  M_IDLE,        // waiting, idle animation
  M_STARTING,    // start sweep
  M_PLAYING,     // cursor running, clock running
  M_FX,          // hit / miss effect (game + clock paused)
  M_ENDING,      // game-over / win effect
  M_TEST,        // effect preview from the web page
  M_UPDATING     // firmware upload in progress
};

enum EndKind : uint8_t { END_OVER, END_BEST, END_WIN };

enum PlayerMode : uint8_t { PM_SOLO, PM_DUEL, PM_TURNS };

static const char* const MODE_NAMES[] = {
  "idle", "starting", "playing", "fx", "ending", "test", "updating"
};

static const char* const PLAYER_NAMES[PLAYER_MODES] = {
  "Solo", "Duel (2 buttons)", "Turns (2 players, 1 button)"
};

struct ActiveFx {
  uint8_t  style;
  CRGB     color;
  uint16_t halfMs;
  uint32_t dur;
  uint32_t start;
  int      center;
};

// One player's round. Solo uses pr[0]; turns uses pr[turn]; duel uses both.
struct PlayerRound {
  int      score;
  uint16_t hits;
  uint16_t misses;
  uint16_t early;          // misses where the cursor had not reached the target
  uint16_t late;           // misses where it had already passed
  uint8_t  streak;
  uint8_t  bestStreak;
};

// Result of the last finished round, shown on the web Scores tab.
struct LastRound {
  bool        valid;
  uint8_t     playerMode;
  int8_t      winner;      // duel / turns: 0, 1, or -1 for a draw
  PlayerRound p[2];
};

Mode mode = M_IDLE;

ActiveFx fx;

PlayerRound pr[2];
LastRound   lastRound;

uint8_t turn = 0;          // turns mode: 0 = player 1's round, 1 = player 2's

int  cursorPosition = 0;
int  targetPosition = 0;
int  highScore      = 0;
int8_t dirStep      = 1;

uint8_t endKind     = END_OVER;

uint32_t modeStart    = 0;
uint32_t lastMoveTime = 0;
uint32_t roundEndMs   = 0;
uint32_t lastDrawMs   = 0;

int lastShownSec = -1;
int lastTickSec  = -1;

bool startTestOnly = false;

bool     settingsDirty   = false;
uint32_t settingsDirtyAt = 0;
uint16_t settingsVer     = 0;      // bumps on every change so other phones can refresh

uint16_t lbVer     = 0;
int8_t   lbPending = -1;           // leaderboard rank waiting for initials

uint8_t  pinFails     = 0;
uint32_t pinLockUntil = 0;

uint32_t rebootAt = 0;

bool oledIsIdle = false;           // the idle screen is up (safe to redraw it)

#define STA_TIMEOUT_MS  20000      // stop looking for the home WiFi after this
bool staGaveUp = false;

// buttons
struct Button {
  uint8_t  pin;
  bool     state;
  uint32_t lockUntil;
};

Button buttons[2] = {
  { BUTTON_PIN,  HIGH, 0 },
  { BUTTON2_PIN, HIGH, 0 }
};

// ============================================================
// HELPERS
// ============================================================

CRGB rgb(uint32_t c) {

  return CRGB((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

bool isDuel() { return cfg.playerMode == PM_DUEL; }

bool isTurns() { return cfg.playerMode == PM_TURNS; }

// The player whose round it is (duel: player 1, only used for display).
uint8_t activePlayer() { return isTurns() ? turn : 0; }

// Score that drives speed and level: in a duel both players push the pace.
int paceScore() {

  return isDuel() ? pr[0].score + pr[1].score : pr[activePlayer()].score;
}

int level() {

  return paceScore() / max(1, (int)cfg.pointsPerLevel) + 1;
}

int comboMult(uint8_t p) {

  if (!cfg.comboEvery)
    return 1;

  return min(COMBO_MAX, 1 + pr[p].streak / cfg.comboEvery);
}

int circDist(int a, int b) {

  int d = abs(a - b);

  int n = cfg.ledCount;

  return d > n - d ? n - d : d;
}

// Steps from the target to the cursor in the direction of travel:
// < 0 the cursor had not reached the target yet (early), > 0 it had passed (late).
int signedOffset() {

  int n = cfg.ledCount;

  int d = ((cursorPosition - targetPosition) * dirStep % n + n) % n;

  return d > n / 2 ? d - n : d;
}

int getCurrentSpeed() {

  int floorMs = min((int)cfg.minDelay, (int)cfg.speedDelay);

  int d = (int)cfg.speedDelay - paceScore() * (int)cfg.speedStep;

  return d < floorMs ? floorMs : d;
}

int32_t timeLeftMs() {

  uint32_t ref = (mode == M_FX) ? fx.start : millis();

  int32_t v = (int32_t)(roundEndMs - ref);

  return v < 0 ? 0 : v;
}

int accuracy(const PlayerRound& p) {

  int presses = p.hits + p.misses;

  return presses ? (p.hits * 100 + presses / 2) / presses : 0;
}

void showLeds() {

  for (int i = cfg.ledCount; i < MAX_LEDS; i++)
    leds[i] = CRGB::Black;

  FastLED.setBrightness(cfg.brightness);

  FastLED.show();
}

void clearLEDs() {

  fill_solid(leds, MAX_LEDS, CRGB::Black);

  showLeds();
}

String localIp() {

  if (WiFi.status() == WL_CONNECTED)
    return WiFi.localIP().toString();

  return WiFi.softAPIP().toString();
}

// ============================================================
// OLED
// ============================================================

void oledMessage(const char* line1, const char* line2, const char* line3) {

  oledIsIdle = false;

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(line1);

  display.setTextSize(1);
  display.setCursor(0, 30);
  display.println(line2);

  display.setCursor(0, 45);
  display.println(line3);

  display.display();
}

void oledIdle() {

  oledIsIdle = true;

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  // turns mode, player 1 done: hand over to player 2
  if (isTurns() && turn == 1) {

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("P1: ");
    display.print(pr[0].score);

    display.setTextSize(1);
    display.setCursor(0, 24);
    display.print("Player 2, your turn!");

    display.setCursor(0, 40);
    display.print("Score more than ");
    display.print(pr[0].score);

    display.setCursor(0, 56);
    display.print("Press button to play");

    display.display();

    return;
  }

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("CYCLONE");

  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print(isDuel()  ? "DUEL   2 buttons"
              : isTurns() ? "TURNS  P1 then P2"
                          : "TARGET LOCK  v2");

  display.setCursor(0, 33);
  display.print("BEST: ");
  display.print(highScore);

  display.setCursor(0, 45);
  display.print("Press button to play");

  display.setCursor(0, 56);
  display.print(localIp());

  display.display();
}

void printTime(int remSec, int x, int y, uint8_t size) {

  display.setTextSize(size);
  display.setCursor(x, y);

  if (remSec >= 100) {

    display.print(remSec / 60);
    display.print(':');
    if (remSec % 60 < 10) display.print('0');
    display.print(remSec % 60);

  } else {

    display.print(remSec);
  }
}

void drawTimeBar(int32_t left) {

  uint32_t total = max(1, (int)cfg.roundSeconds) * 1000UL;

  int w = (int)((uint64_t)left * 124 / total);

  if (w > 124) w = 124;

  display.drawRect(0, 52, 128, 12, SSD1306_WHITE);
  display.fillRect(2, 54, w, 8, SSD1306_WHITE);
}

void oledDuel(int remSec, int32_t left) {

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("P1");

  display.setCursor(116, 0);
  display.print("P2");

  display.setTextSize(3);
  display.setCursor(0, 10);
  display.print(pr[0].score);

  // right-align player 2's score
  int s2 = pr[1].score;
  int digits = s2 >= 100 ? 3 : s2 >= 10 ? 2 : 1;

  display.setCursor(128 - digits * 18, 10);
  display.print(s2);

  printTime(remSec, remSec >= 100 ? 52 : 56, 16, remSec >= 100 ? 1 : 2);

  display.setTextSize(1);

  for (uint8_t p = 0; p < 2; p++) {

    int m = comboMult(p);

    if (m > 1) {

      display.setCursor(p ? 110 : 0, 40);
      display.print('x');
      display.print(m);
    }
  }

  display.setCursor(46, 40);
  display.print("LV ");
  display.print(level());

  drawTimeBar(left);
}

void updateOLED() {

  int32_t left = timeLeftMs();

  int remSec = (left + 999) / 1000;

  lastShownSec = remSec;

  oledIsIdle = false;

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  if (isDuel()) {

    oledDuel(remSec, left);

    display.display();

    return;
  }

  const PlayerRound& p = pr[activePlayer()];

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(isTurns() ? (turn ? "P2 SCORE" : "P1 SCORE") : "SCORE");

  int m = comboMult(activePlayer());

  if (m > 1) {

    display.setCursor(56, 0);
    display.print('x');
    display.print(m);
  }

  display.setCursor(80, 0);
  display.print("TIME");

  display.setTextSize(3);
  display.setCursor(0, 10);
  display.print(p.score);

  if (remSec >= 100)
    printTime(remSec, 80, 14, 2);
  else
    printTime(remSec, 80, 10, 3);

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("BEST ");
  display.print(highScore);

  display.setCursor(80, 40);
  display.print("LV ");
  display.print(level());

  drawTimeBar(left);

  display.display();
}

void oledResult(uint8_t kind) {

  const PlayerRound& p = pr[activePlayer()];

  const char* title = kind == END_WIN  ? "YOU WIN!"
                    : kind == END_BEST ? "NEW BEST!"
                                       : "GAME OVER";

  oledIsIdle = false;

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(title);

  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("SCORE");

  display.setTextSize(3);
  display.setCursor(0, 31);
  display.print(p.score);

  display.setTextSize(1);
  display.setCursor(84, 22);
  display.print("BEST");

  display.setTextSize(2);
  display.setCursor(84, 33);
  display.print(highScore);

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("HIT ");
  display.print(accuracy(p));
  display.print("%  STREAK ");
  display.print(p.bestStreak);

  display.display();
}

// Final screen of a duel or of player 2's turn.
void oledVersus(int8_t winner) {

  oledIsIdle = false;

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(winner < 0 ? "DRAW!" : winner == 0 ? "P1 WINS!" : "P2 WINS!");

  display.setTextSize(1);
  display.setCursor(0, 22);
  display.print("P1");

  display.setCursor(116, 22);
  display.print("P2");

  display.setTextSize(3);
  display.setCursor(0, 31);
  display.print(pr[0].score);

  int s2 = pr[1].score;
  int digits = s2 >= 100 ? 3 : s2 >= 10 ? 2 : 1;

  display.setCursor(128 - digits * 18, 31);
  display.print(s2);

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("Press button to play");

  display.display();
}

// ============================================================
// EFFECT PLAYER  (non-blocking)
// ============================================================

// Begin an effect. Returns false (and changes nothing) if it has no duration.
bool startFx(uint8_t style, uint32_t color, uint8_t blinks, uint16_t ms,
             int center, uint32_t maxDur, Mode next) {

  uint32_t dur = 2UL * ms * blinks;

  if (dur > maxDur)
    dur = maxDur;

  if (dur == 0)
    return false;

  fx.style  = style;
  fx.color  = rgb(color);
  fx.halfMs = ms;
  fx.dur    = dur;
  fx.start  = millis();
  fx.center = center;

  mode = next;

  return true;
}

// ============================================================
// GAME
// ============================================================

void generateTarget() {

  int n = cfg.ledCount;

  if (n <= 1)
    return;

  int t;
  int guard = 0;

  // never drop the target on top of (or right next to) the cursor
  do {

    t = random(0, n);

    guard++;

  } while (
    (t == targetPosition || circDist(t, cursorPosition) <= cfg.hitWindow + 1) &&
    guard < 30
  );

  targetPosition = t;
}

void drawGame() {

  int n = cfg.ledCount;

  CRGB bg  = rgb(cfg.cBg);
  CRGB cur = rgb(cfg.cCursor);

  fill_solid(leds, MAX_LEDS, bg);

  // comet tail behind the cursor
  for (int t = cfg.tailLen; t >= 1; t--) {

    int idx = ((cursorPosition - dirStep * t) % n + n) % n;

    leds[idx] = blend(bg, cur, 255 * (cfg.tailLen + 1 - t) / (cfg.tailLen + 2));
  }

  // target (optionally pulsing)
  CRGB tc = rgb(cfg.cTarget);

  if (cfg.targetPulse)
    tc.nscale8(beatsin8(70, 90, 255));

  leds[targetPosition % n] = tc;

  leds[cursorPosition % n] = cur;

  showLeds();

  lastDrawMs = millis();
}

void startGame() {

  // a fresh start in turns mode (or any other mode) resets both players
  if (!isTurns() || turn == 0)
    memset(pr, 0, sizeof(pr));
  else
    memset(&pr[1], 0, sizeof(pr[1]));

  cursorPosition = 0;
  startTestOnly  = false;
  lbPending      = -1;

  dirStep = (cfg.dirMode == 1) ? -1 : 1;

  generateTarget();

  soundPlay(cfg.sndStart);

  if (isTurns())
    oledMessage(turn ? "PLAYER 2" : "PLAYER 1", "GET READY!", "Lock it in!");
  else if (isDuel())
    oledMessage("DUEL", "GET READY!", "First to lock it wins");
  else
    oledMessage("TARGET", "GET READY!", "Lock it in!");

  mode      = M_STARTING;
  modeStart = millis();
}

void beginPlaying() {

  uint32_t now = millis();

  mode         = M_PLAYING;
  lastMoveTime = now;
  roundEndMs   = now + (uint32_t)cfg.roundSeconds * 1000UL;
  lastTickSec  = -1;

  drawGame();

  updateOLED();
}

void finishEnd() {

  mode = M_IDLE;

  clearLEDs();
}

// Put a finished player's round on the leaderboard (solo and turns only).
void recordScore(uint8_t p, const char* name) {

  LbEntry e;

  memset(&e, 0, sizeof(e));

  strncpy(e.name, name, 3);

  e.score    = pr[p].score;
  e.accuracy = accuracy(pr[p]);
  e.streak   = pr[p].bestStreak;

  int rank = lbInsert(e);

  if (rank >= 0) {

    lbPending = rank;

    lbVer++;
  }
}

void saveLastRound(int8_t winner) {

  lastRound.valid      = true;
  lastRound.playerMode = cfg.playerMode;
  lastRound.winner     = winner;

  memcpy(lastRound.p, pr, sizeof(pr));

  lbVer++;
}

void playEnd(bool celebrate) {

  if (celebrate) {

    soundPlay(cfg.sndWin);

    if (!startFx(cfg.winStyle, cfg.cWin, cfg.winBlinks, cfg.winMs,
                 cursorPosition, 15000, M_ENDING))
      finishEnd();

  } else {

    soundPlay(cfg.sndOver);

    if (!startFx(cfg.overStyle, cfg.cOver, cfg.overBlinks, cfg.overMs,
                 cursorPosition, 15000, M_ENDING))
      finishEnd();
  }
}

// winner: the player who reached the win score (duel), otherwise ignored
void endGame(uint8_t kind, int8_t winner = -1) {

  // ---- duel: compare the two players ----
  if (isDuel()) {

    if (kind != END_WIN)
      winner = pr[0].score > pr[1].score ? 0 : pr[1].score > pr[0].score ? 1 : -1;

    saveLastRound(winner);

    oledVersus(winner);

    playEnd(winner >= 0);

    return;
  }

  uint8_t p = activePlayer();

  bool newBest = pr[p].score > highScore;

  if (newBest) {

    highScore = pr[p].score;

    highScoreSave(highScore);
  }

  recordScore(p, isTurns() ? (p ? "P2" : "P1") : "---");

  // ---- turns: player 1 done, hand over ----
  if (isTurns() && turn == 0) {

    saveLastRound(-1);

    turn = 1;

    oledIdle();

    playEnd(newBest);

    return;
  }

  // ---- turns: player 2 done, pick the winner ----
  if (isTurns()) {

    winner = pr[0].score > pr[1].score ? 0 : pr[1].score > pr[0].score ? 1 : -1;

    saveLastRound(winner);

    turn = 0;

    oledVersus(winner);

    playEnd(winner >= 0);

    return;
  }

  // ---- solo ----
  if (kind == END_OVER && newBest)
    kind = END_BEST;

  endKind = kind;

  saveLastRound(-1);

  oledResult(kind);

  playEnd(kind != END_OVER);
}

void abortGame() {

  soundStop();

  turn = 0;

  mode = M_IDLE;

  clearLEDs();
}

void judgePress(uint8_t p) {

  PlayerRound& me = pr[p];

  int off = signedOffset();

  bool hit = abs(off) <= cfg.hitWindow;

  int center = cursorPosition;

  if (hit) {

    int oldLevel = level();

    me.score += cfg.hitPoints * comboMult(p);

    me.hits++;

    if (me.streak < 255) me.streak++;

    if (me.streak > me.bestStreak) me.bestStreak = me.streak;

    if (cfg.timeBonus)
      roundEndMs += cfg.timeBonus * 1000UL;

    if (cfg.winScore && me.score >= cfg.winScore) {

      endGame(END_WIN, p);

      return;
    }

    if (cfg.dirMode == 2)
      dirStep = -dirStep;
    else if (cfg.dirMode == 3)
      dirStep = random(2) ? 1 : -1;

    soundPlay(level() > oldLevel ? cfg.sndLevel : cfg.sndHit);

    generateTarget();

    uint32_t color = (isDuel() && p == 1) ? cfg.cP2 : cfg.cHit;

    if (!startFx(cfg.hitStyle, color, cfg.hitBlinks, cfg.hitMs,
                 center, 2000, M_FX))
      drawGame();

  } else {

    me.score = max(0, me.score - (int)cfg.missPenalty);

    me.misses++;

    me.streak = 0;

    if (off < 0) me.early++; else me.late++;

    if (cfg.timePenalty) {

      uint32_t cut = cfg.timePenalty * 1000UL;

      roundEndMs = (timeLeftMs() > (int32_t)cut) ? roundEndMs - cut : millis();
    }

    soundPlay(cfg.sndMiss);

    generateTarget();

    if (!startFx(cfg.missStyle, cfg.cMiss, cfg.missBlinks, cfg.missMs,
                 center, 2000, M_FX))
      drawGame();
  }

  updateOLED();
}

void onPress(uint8_t button) {

  // the second button only plays in a duel
  if (button == 1 && !isDuel())
    return;

  if (mode == M_IDLE)
    startGame();
  else if (mode == M_PLAYING)
    judgePress(isDuel() ? button : activePlayer());
}

// Edge-triggered: the press counts the moment the contact closes, so the
// cursor position judged is the one the player saw. Bounce is ignored for
// BUTTON_LOCK_MS afterwards.
bool buttonPressed(Button& b) {

  uint32_t now = millis();

  if ((int32_t)(now - b.lockUntil) < 0)
    return false;

  bool reading = digitalRead(b.pin);

  if (reading == b.state)
    return false;

  // filter a single-sample spike on a long wire
  if (reading == LOW) {

    delayMicroseconds(200);

    if (digitalRead(b.pin) != LOW)
      return false;
  }

  b.state     = reading;
  b.lockUntil = now + BUTTON_LOCK_MS;

  return reading == LOW;
}

void checkButtons() {

  for (uint8_t i = 0; i < 2; i++)
    if (buttonPressed(buttons[i]))
      onPress(i);
}

// ============================================================
// PER-MODE TICKS
// ============================================================

void tickIdle() {

  static uint32_t last = 0;

  uint32_t now = millis();

  if (now - last < 25)
    return;

  last = now;

  idleFrame(cfg.idleMode, rgb(cfg.cIdle), cfg.idleSpeed, cfg.ledCount);

  showLeds();
}

void tickStarting() {

  uint32_t elapsed = millis() - modeStart;

  uint32_t sweepMs = (uint32_t)cfg.startSweep * cfg.ledCount;

  sweepFrame(rgb(cfg.cStart), elapsed, cfg.startSweep, cfg.ledCount);

  showLeds();

  if (elapsed >= sweepMs + 250) {

    if (startTestOnly) {

      startTestOnly = false;

      mode = M_IDLE;

      clearLEDs();

      oledIdle();

    } else {

      beginPlaying();
    }
  }
}

void tickPlaying() {

  uint32_t now = millis();

  // round over?
  if (timeLeftMs() == 0) {

    endGame(END_OVER);

    return;
  }

  // cursor
  if (now - lastMoveTime >= (uint32_t)getCurrentSpeed()) {

    lastMoveTime = now;

    cursorPosition = (cursorPosition + dirStep + cfg.ledCount) % cfg.ledCount;

    if (cfg.stepClick && !soundBusy())
      tone(BUZZER_PIN, 2600, 4);

    drawGame();

  } else if (cfg.targetPulse && now - lastDrawMs > 33) {

    drawGame();
  }

  // once per second: OLED refresh + countdown ticks
  int remSec = (timeLeftMs() + 999) / 1000;

  if (remSec != lastShownSec) {

    updateOLED();

    if (cfg.tickSec && remSec <= cfg.tickSec && remSec > 0 && remSec != lastTickSec) {

      lastTickSec = remSec;

      soundPlay(cfg.sndTick);
    }
  }
}

void tickFx() {

  static uint32_t lastFrame = 0;

  uint32_t now = millis();

  uint32_t elapsed = now - fx.start;

  if (elapsed >= fx.dur) {

    switch (mode) {

      case M_FX:
        roundEndMs  += fx.dur;    // effects do not eat game time
        lastMoveTime = now;
        mode = M_PLAYING;
        drawGame();
        updateOLED();
        break;

      case M_ENDING:
        finishEnd();
        break;

      default:
        mode = M_IDLE;
        clearLEDs();
        oledIdle();
        break;
    }

    return;
  }

  if (now - lastFrame < 12)
    return;

  lastFrame = now;

  fxFrame(fx.style, fx.color, elapsed, fx.halfMs, fx.center, cfg.ledCount);

  showLeds();
}

// ============================================================
// WEB API
// ============================================================

void sendJsonCode(int code, const String& body) {

  server.sendHeader("Cache-Control", "no-store");

  server.send(code, "application/json", body);
}

void sendJson(const String& body) {

  sendJsonCode(200, body);
}

// Admin routes need the PIN once one is set. Five wrong tries lock them
// for a minute, and every wrong try after that locks them again.
bool requirePin() {

  if (!net.adminPin[0])
    return true;

  if (pinFails >= 5 && (int32_t)(millis() - pinLockUntil) < 0) {

    sendJsonCode(429, "{\"ok\":false,\"error\":\"Too many wrong PINs. Wait a minute.\"}");

    return false;
  }

  if (server.arg("pin") == net.adminPin) {

    pinFails = 0;

    return true;
  }

  if (pinFails < 255) pinFails++;

  if (pinFails >= 5)
    pinLockUntil = millis() + 60000;

  sendJsonCode(401, "{\"ok\":false,\"error\":\"pin\"}");

  return false;
}

void jsonString(String& j, const char* s) {

  j += '"';

  for (; *s; s++) {

    if (*s == '"' || *s == '\\') j += '\\';

    if ((uint8_t)*s >= 0x20) j += *s;
  }

  j += '"';
}

String namesJson(const char* const* names, int count) {

  String j = "[";

  for (int i = 0; i < count; i++) {

    if (i) j += ',';

    jsonString(j, names[i]);
  }

  return j + ']';
}

String stateJson() {

  String j;

  j.reserve(360);

  bool live = (mode == M_PLAYING || mode == M_FX);

  uint8_t ap = activePlayer();

  j += "{\"mode\":\"";
  j += MODE_NAMES[mode];
  j += "\",\"score\":";
  j += pr[ap].score;
  j += ",\"p\":[";
  j += pr[0].score;
  j += ',';
  j += pr[1].score;
  j += "],\"players\":";
  j += cfg.playerMode;
  j += ",\"turn\":";
  j += turn;
  j += ",\"mult\":[";
  j += comboMult(0);
  j += ',';
  j += comboMult(1);
  j += "],\"best\":";
  j += highScore;
  j += ",\"level\":";
  j += level();
  j += ",\"timeLeft\":";
  j += live ? (int)((timeLeftMs() + 999) / 1000) : (int)cfg.roundSeconds;
  j += ",\"round\":";
  j += cfg.roundSeconds;
  j += ",\"saved\":";
  j += settingsDirty ? "false" : "true";
  j += ",\"sv\":";
  j += settingsVer;
  j += ",\"lbv\":";
  j += lbVer;
  j += ",\"pending\":";
  j += lbPending;
  j += ",\"clients\":";
  j += WiFi.softAPgetStationNum();
  j += ",\"heap\":";
  j += ESP.getFreeHeap();
  j += ",\"up\":";
  j += millis() / 1000;
  j += '}';

  return j;
}

String settingsJson() {

  String j;

  j.reserve(3600);

  j += "{\"v\":";
  j += settingsValuesJson();

  j += ",\"limits\":";
  j += settingsLimitsJson();

  j += ",\"meta\":{\"styles\":";
  j += namesJson(STYLE_NAMES, STYLE_COUNT);
  j += ",\"idle\":";
  j += namesJson(IDLE_NAMES, IDLE_COUNT);
  j += ",\"dir\":";
  j += namesJson(DIR_NAMES, DIR_COUNT);
  j += ",\"players\":";
  j += namesJson(PLAYER_NAMES, PLAYER_MODES);

  j += ",\"sounds\":[";
  for (int i = 0; i < SOUND_COUNT; i++) {
    if (i) j += ',';
    jsonString(j, soundName(i));
  }

  j += "],\"presets\":[";
  for (int i = 0; i < PRESET_COUNT; i++) {
    if (i) j += ',';
    j += '[';
    jsonString(j, presetName(i));
    j += ',';
    jsonString(j, presetDesc(i));
    j += ']';
  }
  j += "]}";

  j += ",\"slots\":[";
  for (int i = 0; i < SLOT_COUNT; i++) {
    if (i) j += ',';
    j += slotUsed(i) ? "true" : "false";
  }
  j += "],\"sv\":";
  j += settingsVer;

  // network info (never the passwords)
  j += ",\"net\":{\"ap\":";
  jsonString(j, apSsid);
  j += ",\"apIp\":";
  jsonString(j, WiFi.softAPIP().toString().c_str());
  j += ",\"host\":";
  jsonString(j, net.host);
  j += ",\"sta\":";
  jsonString(j, net.staSsid);
  j += ",\"staGaveUp\":";
  j += staGaveUp ? "true" : "false";
  j += ",\"staOk\":";
  j += WiFi.status() == WL_CONNECTED ? "true" : "false";
  j += ",\"staIp\":";
  jsonString(j, WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "");
  j += ",\"pin\":";
  j += net.adminPin[0] ? "true" : "false";
  j += ",\"fw\":";
  jsonString(j, __DATE__ " " __TIME__);
  j += "}}";

  return j;
}

String roundJson(const PlayerRound& p) {

  String j = "{\"score\":";
  j += p.score;
  j += ",\"hits\":";
  j += p.hits;
  j += ",\"misses\":";
  j += p.misses;
  j += ",\"early\":";
  j += p.early;
  j += ",\"late\":";
  j += p.late;
  j += ",\"streak\":";
  j += p.bestStreak;
  j += ",\"acc\":";
  j += accuracy(p);
  j += '}';

  return j;
}

String scoresJson() {

  String j;

  j.reserve(600);

  j += "{\"lb\":[";

  bool first = true;

  for (int i = 0; i < LB_COUNT; i++) {

    if (!lb.e[i].score)
      break;

    if (!first) j += ',';

    first = false;

    j += "{\"n\":";
    jsonString(j, lb.e[i].name);
    j += ",\"s\":";
    j += lb.e[i].score;
    j += ",\"a\":";
    j += lb.e[i].accuracy;
    j += ",\"k\":";
    j += lb.e[i].streak;
    j += '}';
  }

  j += "],\"pending\":";
  j += lbPending;

  j += ",\"last\":";

  if (lastRound.valid) {

    j += "{\"players\":";
    j += lastRound.playerMode;
    j += ",\"winner\":";
    j += lastRound.winner;
    j += ",\"p\":[";
    j += roundJson(lastRound.p[0]);
    j += ',';
    j += roundJson(lastRound.p[1]);
    j += "]}";

  } else {

    j += "null";
  }

  j += '}';

  return j;
}

void markDirty() {

  settingsDirty   = true;
  settingsDirtyAt = millis();

  settingsVer++;
}

// keep positions valid after the LED count / direction / players changed
void settingsChanged() {

  static uint8_t lastPlayers = 255;

  if (cfg.ledCount < 1)
    cfg.ledCount = 1;

  cursorPosition %= cfg.ledCount;
  targetPosition %= cfg.ledCount;

  if (cfg.dirMode < 2)
    dirStep = (cfg.dirMode == 1) ? -1 : 1;

  // switching the player mode mid-game ends the game
  if (lastPlayers != 255 && lastPlayers != cfg.playerMode) {

    if (mode != M_IDLE)
      abortGame();

    turn = 0;

    memset(pr, 0, sizeof(pr));

    oledIdle();
  }

  lastPlayers = cfg.playerMode;

  markDirty();

  if (mode == M_PLAYING) {

    drawGame();

    updateOLED();
  }
}

void handleRoot() {

  server.send_P(200, "text/html", INDEX_HTML);
}

void handleState()    { sendJson(stateJson()); }

void handleSettings() { sendJson(settingsJson()); }

void handleScores()   { sendJson(scoresJson()); }

void handleSet() {

  for (int i = 0; i < server.args(); i++)
    settingsSetField(server.argName(i), server.arg(i));

  settingsChanged();

  String j = "{\"ok\":true,\"sv\":";
  j += settingsVer;
  j += '}';

  sendJson(j);
}

void handleStart() {

  if (mode == M_IDLE || mode == M_PLAYING || mode == M_FX) {

    // restarting mid-round in turns mode starts over with player 1
    if (mode != M_IDLE)
      turn = 0;

    startGame();
  }

  sendJson(stateJson());
}

void handleStop() {

  if (mode != M_IDLE || turn) {

    abortGame();

    oledMessage("STOPPED", "Press button", "or use the web");
  }

  sendJson(stateJson());
}

void handleReset() {

  abortGame();

  memset(pr, 0, sizeof(pr));

  oledIdle();

  sendJson(stateJson());
}

void handlePreset() {

  settingsApplyPreset(server.arg("id").toInt());

  settingsChanged();

  sendJson("{\"ok\":true}");
}

void handleSlot() {

  int i = server.arg("i").toInt();

  String op = server.arg("op");

  bool ok = false;

  if (op == "save") {

    ok = slotSave(i);

    if (ok)
      settingsVer++;

  } else if (op == "load") {

    ok = slotLoad(i);

    if (ok)
      settingsChanged();
  }

  sendJsonCode(ok ? 200 : 400, ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void handleTest() {

  if (server.hasArg("sound")) {

    soundPlay(server.arg("sound").toInt(), true);

    sendJson("{\"ok\":true}");

    return;
  }

  if (mode != M_IDLE) {

    sendJsonCode(409, "{\"ok\":false,\"error\":\"stop the game first\"}");

    return;
  }

  String kind = server.arg("fx");

  bool ok = true;

  int center = random(cfg.ledCount);

  if (kind == "hit") {

    soundPlay(cfg.sndHit);
    ok = startFx(cfg.hitStyle, cfg.cHit, cfg.hitBlinks, cfg.hitMs, center, 2000, M_TEST);

  } else if (kind == "miss") {

    soundPlay(cfg.sndMiss);
    ok = startFx(cfg.missStyle, cfg.cMiss, cfg.missBlinks, cfg.missMs, center, 2000, M_TEST);

  } else if (kind == "over") {

    soundPlay(cfg.sndOver);
    ok = startFx(cfg.overStyle, cfg.cOver, cfg.overBlinks, cfg.overMs, center, 15000, M_TEST);

  } else if (kind == "win") {

    soundPlay(cfg.sndWin);
    ok = startFx(cfg.winStyle, cfg.cWin, cfg.winBlinks, cfg.winMs, center, 15000, M_TEST);

  } else if (kind == "start") {

    soundPlay(cfg.sndStart);
    startTestOnly = true;
    mode          = M_STARTING;
    modeStart     = millis();

  } else {

    ok = false;
  }

  sendJsonCode(ok ? 200 : 400, ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"effect has 0 blinks\"}");
}

// Initials for the leaderboard entry of the round that just ended.
void handleName() {

  if (lbPending < 0) {

    sendJsonCode(409, "{\"ok\":false,\"error\":\"nothing to name\"}");

    return;
  }

  String raw = server.arg("n");

  raw.toUpperCase();

  char name[4] = { 0 };
  uint8_t len = 0;

  for (unsigned i = 0; i < raw.length() && len < 3; i++) {

    char c = raw[i];

    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
      name[len++] = c;
  }

  if (!len) {

    sendJsonCode(400, "{\"ok\":false,\"error\":\"use letters or digits\"}");

    return;
  }

  memcpy(lb.e[lbPending].name, name, 4);

  lbSave();

  lbPending = -1;

  lbVer++;

  sendJson(scoresJson());
}

void handleResetHigh() {

  if (!requirePin())
    return;

  highScore = 0;

  highScoreSave(0);

  lbClear();

  lbSave();

  lbPending = -1;

  lbVer++;

  sendJson(stateJson());
}

void handleFactory() {

  if (!requirePin())
    return;

  abortGame();

  settingsDefaults(cfg);

  settingsClamp();

  settingsSave();

  settingsChanged();

  sendJson("{\"ok\":true}");
}

void handleReboot() {

  if (!requirePin())
    return;

  sendJson("{\"ok\":true}");

  rebootAt = millis() + 500;
}

bool validHost(const String& h) {

  if (h.length() < 1 || h.length() > 24 || h[0] == '-')
    return false;

  for (unsigned i = 0; i < h.length(); i++) {

    char c = h[i];

    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))
      return false;
  }

  return true;
}

bool validPin(const String& p) {

  if (p.length() == 0)
    return true;          // no PIN

  if (p.length() < 4 || p.length() > 8)
    return false;

  for (unsigned i = 0; i < p.length(); i++)
    if (p[i] < '0' || p[i] > '9')
      return false;

  return true;
}

// WiFi + admin settings. Only the fields sent are changed. Reboots to apply.
void handleNet() {

  if (!requirePin())
    return;

  NetConfig next = net;

  String err;

  if (server.hasArg("apPass")) {

    String v = server.arg("apPass");

    if (v.length() < 8 || v.length() > 32)
      err = "WiFi password must be 8 to 32 characters";
    else
      strcpy(next.apPass, v.c_str());
  }

  if (server.hasArg("staSsid")) {

    String v = server.arg("staSsid");

    if (v.length() > 32)
      err = "Home WiFi name is too long";
    else
      strcpy(next.staSsid, v.c_str());

    if (!v.length())
      next.staPass[0] = 0;
  }

  if (server.hasArg("staPass")) {

    String v = server.arg("staPass");

    if (v.length() > 64)
      err = "Home WiFi password is too long";
    else
      strcpy(next.staPass, v.c_str());
  }

  if (server.hasArg("host")) {

    String v = server.arg("host");

    v.toLowerCase();

    if (!validHost(v))
      err = "Name: 1 to 24 of a-z, 0-9 and -";
    else
      strcpy(next.host, v.c_str());
  }

  if (server.hasArg("newPin")) {

    String v = server.arg("newPin");

    if (!validPin(v))
      err = "PIN must be 4 to 8 digits (or empty for none)";
    else
      strcpy(next.adminPin, v.c_str());
  }

  if (err.length()) {

    String j = "{\"ok\":false,\"error\":";
    jsonString(j, err.c_str());
    j += '}';

    sendJsonCode(400, j);

    return;
  }

  net = next;

  netSave();

  sendJson("{\"ok\":true,\"reboot\":true}");

  rebootAt = millis() + 800;
}

// Captive portal: phones probe a known URL after joining the AP. Anything
// that is not ours gets sent to the control page so it opens by itself.
void handleNotFound() {

  String host = server.hostHeader();

  bool ours = host == WiFi.softAPIP().toString() ||
              host == WiFi.localIP().toString() ||
              host.equalsIgnoreCase(String(net.host) + ".local");

  server.sendHeader("Location", ours ? String("/")
                                     : "http://" + WiFi.softAPIP().toString() + "/");

  server.send(302, "text/plain", "");
}

// ============================================================
// LIVE STATE OVER WEBSOCKET
// ============================================================

void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {

  if (type == WStype_CONNECTED) {

    String s = stateJson();

    ws.sendTXT(num, s);
  }
}

// Push the state when something the page shows has changed (checked every
// 50 ms), and at least once a second so the clock and uptime stay live.
void pushState() {

  static uint32_t lastCheck = 0;
  static uint32_t lastSent  = 0;
  static uint32_t lastSig   = 0;

  uint32_t now = millis();

  if (now - lastCheck < 50 || !ws.connectedClients())
    return;

  lastCheck = now;

  bool live = (mode == M_PLAYING || mode == M_FX);

  uint32_t sig = mode;
  sig = sig * 31 + pr[0].score;
  sig = sig * 31 + pr[1].score;
  sig = sig * 31 + (live ? (timeLeftMs() + 999) / 1000 : 0);
  sig = sig * 31 + highScore;
  sig = sig * 31 + turn;
  sig = sig * 31 + pr[0].streak + pr[1].streak * 7;
  sig = sig * 31 + settingsDirty;
  sig = sig * 31 + settingsVer;
  sig = sig * 31 + lbVer;
  sig = sig * 31 + (uint8_t)lbPending;

  if (sig == lastSig && now - lastSent < 1000)
    return;

  lastSig  = sig;
  lastSent = now;

  String s = stateJson();

  ws.broadcastTXT(s);
}

// ============================================================
// HOME WIFI
// ============================================================

// While the ESP8266 searches for a router it hops channels, which knocks
// phones off the hotspot. If the home WiFi is not found in time, stop
// looking (a reboot tries again). Also refresh the OLED once it connects,
// so the idle screen shows the address to use.
void checkHomeWifi() {

  static bool wasConnected  = false;
  static bool everConnected = false;

  if (!net.staSsid[0] || staGaveUp)
    return;

  bool connected = WiFi.status() == WL_CONNECTED;

  if (connected)
    everConnected = true;

  if (connected != wasConnected) {

    wasConnected = connected;

    if (connected) {

      Serial.print("Home WiFi IP: ");
      Serial.println(WiFi.localIP());
    }

    if (oledIsIdle)
      oledIdle();

    settingsVer++;             // the System tab shows the new status
  }

  // a drop after it has worked is left to the automatic reconnect
  if (!everConnected && millis() > STA_TIMEOUT_MS) {

    staGaveUp = true;

    WiFi.setAutoReconnect(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    Serial.println("Home WiFi not found, hotspot only (reboot to retry)");

    settingsVer++;
  }
}

// ============================================================
// SETUP
// ============================================================

// Holding the button through power-on for 3 s restores the WiFi password,
// removes the admin PIN and forgets the home WiFi.
void checkRecovery() {

  if (digitalRead(BUTTON_PIN) != LOW)
    return;

  oledMessage("RESET?", "Keep holding 3 s to", "clear PIN + WiFi");

  uint32_t t0 = millis();

  while (millis() - t0 < 3000) {

    if (digitalRead(BUTTON_PIN) != LOW) {

      oledMessage("CYCLONE", "Target Lock v2", "Starting...");

      return;
    }

    delay(10);
  }

  netDefaults();

  netSave();

  Serial.println("PIN and WiFi settings cleared");

  oledMessage("CLEARED", "PIN removed, WiFi", "password 12345678");

  soundPlay(4, true);

  // wait for release so the press does not start a game
  while (digitalRead(BUTTON_PIN) == LOW) {

    soundLoop();

    delay(10);
  }

  delay(800);
}

void setupWifi() {

  snprintf(apSsid, sizeof(apSsid), "CycloneGame-%04X", (unsigned)(ESP.getChipId() & 0xFFFF));

  WiFi.persistent(false);

  if (net.staSsid[0]) {

    WiFi.mode(WIFI_AP_STA);

    WiFi.begin(net.staSsid, net.staPass);

  } else {

    WiFi.mode(WIFI_AP);
  }

  WiFi.softAP(apSsid, net.apPass);

  // captive portal: answer every name lookup with our own address
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());
}

void setupOta() {

  ArduinoOTA.setHostname(net.host);

  if (net.adminPin[0])
    ArduinoOTA.setPassword(net.adminPin);

  ArduinoOTA.onStart([]() {

    abortGame();

    mode = M_UPDATING;

    oledMessage("UPDATING", "Do not power off", "");
  });

  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {

    static uint8_t lastPct = 255;

    uint8_t pct = total ? done * 100UL / total : 0;

    if (pct == lastPct)
      return;

    lastPct = pct;

    display.fillRect(0, 52, 128, 12, SSD1306_BLACK);
    display.drawRect(0, 52, 128, 12, SSD1306_WHITE);
    display.fillRect(2, 54, pct * 124 / 100, 8, SSD1306_WHITE);
    display.display();
  });

  ArduinoOTA.onError([](ota_error_t) {

    mode = M_IDLE;

    oledMessage("UPDATE", "failed", "Try again");
  });

  ArduinoOTA.begin();          // also starts mDNS as <host>.local

  MDNS.addService("http", "tcp", 80);

  // browser upload at /update (user "admin", password = the PIN if one is set)
  if (net.adminPin[0])
    httpUpdater.setup(&server, "/update", "admin", net.adminPin);
  else
    httpUpdater.setup(&server, "/update");
}

void setup() {

  Serial.begin(115200);

  delay(300);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  randomSeed(micros());

  settingsBegin();

  highScore = highScoreLoad();

  soundBegin();

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    Serial.println("OLED ERROR");

  oledMessage("CYCLONE", "Target Lock v2", "Starting...");

  checkRecovery();

  // LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, MAX_LEDS);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MA);

  clearLEDs();

  setupWifi();

  Serial.println();
  Serial.println("======================");
  Serial.println("CYCLONE TARGET LOCK v2");
  Serial.print("SSID: ");
  Serial.println(apSsid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("mDNS: http://");
  Serial.print(net.host);
  Serial.println(".local");
  if (net.staSsid[0]) {
    Serial.print("Home WiFi: ");
    Serial.println(net.staSsid);
  }
  Serial.println("======================");

  oledMessage("CYCLONE", apSsid, "Starting...");

  // Web routes
  server.on("/",              handleRoot);
  server.on("/api/state",     handleState);
  server.on("/api/settings",  handleSettings);
  server.on("/api/scores",    handleScores);
  server.on("/api/set",       handleSet);
  server.on("/api/start",     handleStart);
  server.on("/api/stop",      handleStop);
  server.on("/api/reset",     handleReset);
  server.on("/api/preset",    handlePreset);
  server.on("/api/slot",      handleSlot);
  server.on("/api/test",      handleTest);
  server.on("/api/name",      handleName);
  server.on("/api/resethigh", handleResetHigh);
  server.on("/api/factory",   handleFactory);
  server.on("/api/reboot",    handleReboot);
  server.on("/api/net",       handleNet);
  server.onNotFound(handleNotFound);

  setupOta();

  server.begin();

  ws.begin();
  ws.onEvent(wsEvent);

  settingsChanged();
  settingsDirty = false;

  // read the buttons as they are now, so one held at boot is not a press
  for (uint8_t i = 0; i < 2; i++)
    buttons[i].state = digitalRead(buttons[i].pin);

  generateTarget();

  oledIdle();

  soundPlay(1);
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  ArduinoOTA.handle();

  dns.processNextRequest();

  server.handleClient();

  ws.loop();

  if (mode == M_UPDATING)
    return;

  soundLoop();

  checkButtons();

  switch (mode) {

    case M_IDLE:     tickIdle();     break;
    case M_STARTING: tickStarting(); break;
    case M_PLAYING:  tickPlaying();  break;
    default:         tickFx();       break;   // M_FX, M_ENDING, M_TEST
  }

  pushState();

  // save settings shortly after the last change, never during a round
  if (settingsDirty && mode == M_IDLE && millis() - settingsDirtyAt > 1500) {

    settingsSave();

    settingsDirty = false;
  }

  checkHomeWifi();

  if (rebootAt && (int32_t)(millis() - rebootAt) >= 0)
    ESP.restart();
}
