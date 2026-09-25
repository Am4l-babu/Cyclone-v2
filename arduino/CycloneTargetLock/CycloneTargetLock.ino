/*
   ============================================================
        CYCLONE TARGET LOCK  v2  -  ESP8266 LED reaction game
   ============================================================

   A cursor spins around a ring of WS2812B LEDs.
   Press the button the instant it lands on the target LED.
   Hit = points (and the cursor speeds up), miss = penalty.
   A round lasts 60 s by default, then the game ends.
   Everything (round time, colours, effects, sounds, presets)
   is adjustable from the web page.

   Made by Am4l-babu  -  https://github.com/Am4l-babu

   PIN CONNECTIONS
   ------------------------------------------------------------
   WS2812B DATA  -> D6 / GPIO12
   BUTTON        -> D5 / GPIO14  (other leg to GND)
   BUZZER (+)    -> D7 / GPIO13  (other leg to GND, passive buzzer)

   OLED SDA      -> D2 / GPIO4
   OLED SCL      -> D1 / GPIO5
   OLED VCC      -> 3.3V
   OLED GND      -> GND

   WiFi AP:  SSID CycloneGame  /  PASS 12345678
   Web:      http://192.168.4.1

   FILES: main.cpp (this file) + settings, sounds, effects, webui.
   For Arduino IDE run tools/sync_arduino.py (see README).
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

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

#define OLED_SDA      D2
#define OLED_SCL      D1

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

// FastLED power limiter (5 V, mA). Raise it if you use a big 5 V supply.
#define LED_MAX_MA    1500

// ============================================================
// OBJECTS
// ============================================================

CRGB leds[MAX_LEDS];

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

ESP8266WebServer server(80);

const char* AP_SSID = "CycloneGame";
const char* AP_PASS = "12345678";

// ============================================================
// GAME STATE
// ============================================================

enum Mode : uint8_t {
  M_IDLE,        // waiting, idle animation
  M_STARTING,    // start sweep
  M_PLAYING,     // cursor running, clock running
  M_FX,          // hit / miss effect (game + clock paused)
  M_ENDING,      // game-over / win effect
  M_TEST         // effect preview from the web page
};

enum EndKind : uint8_t { END_OVER, END_BEST, END_WIN };

static const char* const MODE_NAMES[] = {
  "idle", "starting", "playing", "fx", "ending", "test"
};

struct ActiveFx {
  uint8_t  style;
  CRGB     color;
  uint16_t halfMs;
  uint32_t dur;
  uint32_t start;
  int      center;
};

Mode mode = M_IDLE;

ActiveFx fx;

int  cursorPosition = 0;
int  targetPosition = 0;
int  score          = 0;
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

// button
bool lastButtonState = HIGH;
bool buttonState     = HIGH;
uint32_t buttonDebounceTime = 0;

// ============================================================
// HELPERS
// ============================================================

CRGB rgb(uint32_t c) {

  return CRGB((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

int level() {

  return score / max(1, (int)cfg.pointsPerLevel) + 1;
}

int circDist(int a, int b) {

  int d = abs(a - b);

  int n = cfg.ledCount;

  return d > n - d ? n - d : d;
}

int getCurrentSpeed() {

  int floorMs = min((int)cfg.minDelay, (int)cfg.speedDelay);

  int d = (int)cfg.speedDelay - score * (int)cfg.speedStep;

  return d < floorMs ? floorMs : d;
}

int32_t timeLeftMs() {

  uint32_t ref = (mode == M_FX) ? fx.start : millis();

  int32_t v = (int32_t)(roundEndMs - ref);

  return v < 0 ? 0 : v;
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

// ============================================================
// OLED
// ============================================================

void oledMessage(const char* line1, const char* line2, const char* line3) {

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

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("CYCLONE");

  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("TARGET LOCK  v2");

  display.setCursor(0, 33);
  display.print("BEST: ");
  display.print(highScore);

  display.setCursor(0, 45);
  display.print("Press button to play");

  display.setCursor(0, 56);
  display.print("192.168.4.1");

  display.display();
}

void updateOLED() {

  int32_t left = timeLeftMs();

  int remSec = (left + 999) / 1000;

  lastShownSec = remSec;

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("SCORE");

  display.setCursor(80, 0);
  display.print("TIME");

  display.setTextSize(3);
  display.setCursor(0, 10);
  display.print(score);

  if (remSec >= 100) {

    display.setTextSize(2);
    display.setCursor(80, 14);
    display.print(remSec / 60);
    display.print(':');
    if (remSec % 60 < 10) display.print('0');
    display.print(remSec % 60);

  } else {

    display.setCursor(80, 10);
    display.print(remSec);
  }

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("BEST ");
  display.print(highScore);

  display.setCursor(80, 40);
  display.print("LV ");
  display.print(level());

  // time bar
  uint32_t total = max(1, (int)cfg.roundSeconds) * 1000UL;

  int w = (int)((uint64_t)left * 124 / total);

  if (w > 124) w = 124;

  display.drawRect(0, 52, 128, 12, SSD1306_WHITE);
  display.fillRect(2, 54, w, 8, SSD1306_WHITE);

  display.display();
}

void oledResult(uint8_t kind) {

  const char* title = kind == END_WIN  ? "YOU WIN!"
                    : kind == END_BEST ? "NEW BEST!"
                                       : "GAME OVER";

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
  display.print(score);

  display.setTextSize(1);
  display.setCursor(84, 22);
  display.print("BEST");

  display.setTextSize(2);
  display.setCursor(84, 33);
  display.print(highScore);

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

  score          = 0;
  cursorPosition = 0;
  startTestOnly  = false;

  dirStep = (cfg.dirMode == 1) ? -1 : 1;

  generateTarget();

  soundPlay(cfg.sndStart);

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

void endGame(uint8_t kind) {

  bool newBest = score > highScore;

  if (kind == END_OVER && newBest)
    kind = END_BEST;

  if (newBest) {

    highScore = score;

    highScoreSave(highScore);
  }

  endKind = kind;

  oledResult(kind);

  if (kind == END_OVER) {

    soundPlay(cfg.sndOver);

    if (!startFx(cfg.overStyle, cfg.cOver, cfg.overBlinks, cfg.overMs,
                 cursorPosition, 15000, M_ENDING))
      finishEnd();

  } else {

    soundPlay(cfg.sndWin);

    if (!startFx(cfg.winStyle, cfg.cWin, cfg.winBlinks, cfg.winMs,
                 cursorPosition, 15000, M_ENDING))
      finishEnd();
  }
}

void abortGame() {

  soundStop();

  mode = M_IDLE;

  clearLEDs();
}

void judgePress() {

  bool hit = circDist(cursorPosition, targetPosition) <= cfg.hitWindow;

  int center = cursorPosition;

  if (hit) {

    int oldLevel = level();

    score += cfg.hitPoints;

    if (cfg.timeBonus)
      roundEndMs += cfg.timeBonus * 1000UL;

    if (cfg.winScore && score >= cfg.winScore) {

      endGame(END_WIN);

      return;
    }

    if (cfg.dirMode == 2)
      dirStep = -dirStep;
    else if (cfg.dirMode == 3)
      dirStep = random(2) ? 1 : -1;

    soundPlay(level() > oldLevel ? cfg.sndLevel : cfg.sndHit);

    generateTarget();

    if (!startFx(cfg.hitStyle, cfg.cHit, cfg.hitBlinks, cfg.hitMs,
                 center, 2000, M_FX))
      drawGame();

  } else {

    score = max(0, score - (int)cfg.missPenalty);

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

void onPress() {

  if (mode == M_IDLE)
    startGame();
  else if (mode == M_PLAYING)
    judgePress();
}

void checkButton() {

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState)
    buttonDebounceTime = millis();

  if (millis() - buttonDebounceTime > 40) {

    if (reading != buttonState) {

      buttonState = reading;

      if (buttonState == LOW)
        onPress();
    }
  }

  lastButtonState = reading;
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

String namesJson(const char* const* names, int count) {

  String j = "[";

  for (int i = 0; i < count; i++) {

    if (i) j += ',';

    j += '"';
    j += names[i];
    j += '"';
  }

  return j + ']';
}

String stateJson() {

  String j;

  j.reserve(300);

  bool live = (mode == M_PLAYING || mode == M_FX);

  j += "{\"mode\":\"";
  j += MODE_NAMES[mode];
  j += "\",\"score\":";
  j += score;
  j += ",\"best\":";
  j += highScore;
  j += ",\"level\":";
  j += level();
  j += ",\"timeLeft\":";
  j += live ? (int)((timeLeftMs() + 999) / 1000) : (int)cfg.roundSeconds;
  j += ",\"round\":";
  j += cfg.roundSeconds;
  j += ",\"saved\":";
  j += settingsDirty ? "false" : "true";
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

  j.reserve(3200);

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

  j += ",\"sounds\":[";
  for (int i = 0; i < SOUND_COUNT; i++) {
    if (i) j += ',';
    j += '"';
    j += soundName(i);
    j += '"';
  }

  j += "],\"presets\":[";
  for (int i = 0; i < PRESET_COUNT; i++) {
    if (i) j += ',';
    j += "[\"";
    j += presetName(i);
    j += "\",\"";
    j += presetDesc(i);
    j += "\"]";
  }
  j += "]}";

  j += ",\"slots\":[";
  for (int i = 0; i < SLOT_COUNT; i++) {
    if (i) j += ',';
    j += slotUsed(i) ? "true" : "false";
  }
  j += "],\"ip\":\"";
  j += WiFi.softAPIP().toString();
  j += "\"}";

  return j;
}

void markDirty() {

  settingsDirty   = true;
  settingsDirtyAt = millis();
}

// keep positions valid after the LED count / direction changed
void settingsChanged() {

  if (cfg.ledCount < 1)
    cfg.ledCount = 1;

  cursorPosition %= cfg.ledCount;
  targetPosition %= cfg.ledCount;

  if (cfg.dirMode < 2)
    dirStep = (cfg.dirMode == 1) ? -1 : 1;

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

void handleSet() {

  for (int i = 0; i < server.args(); i++)
    settingsSetField(server.argName(i), server.arg(i));

  settingsChanged();

  sendJson("{\"ok\":true}");
}

void handleStart() {

  if (mode == M_IDLE || mode == M_PLAYING || mode == M_FX)
    startGame();

  sendJson(stateJson());
}

void handleStop() {

  if (mode != M_IDLE) {

    abortGame();

    oledMessage("STOPPED", "Press button", "or use the web");
  }

  sendJson(stateJson());
}

void handleReset() {

  abortGame();

  score = 0;

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

void handleResetHigh() {

  highScore = 0;

  highScoreSave(0);

  sendJson(stateJson());
}

void handleFactory() {

  abortGame();

  settingsDefaults(cfg);

  settingsClamp();

  settingsSave();

  settingsChanged();

  sendJson("{\"ok\":true}");
}

void handleReboot() {

  sendJson("{\"ok\":true}");

  delay(200);

  ESP.restart();
}

void handleNotFound() {

  server.sendHeader("Location", "/");

  server.send(302);
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(300);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  randomSeed(micros());

  settingsBegin();

  highScore = highScoreLoad();

  soundBegin();

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    Serial.println("OLED ERROR");

  oledMessage("CYCLONE", "Target Lock v2", "Starting...");

  // LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, MAX_LEDS);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MA);

  clearLEDs();

  // WiFi AP
  WiFi.mode(WIFI_AP);

  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.println();
  Serial.println("======================");
  Serial.println("CYCLONE TARGET LOCK v2");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("======================");

  // Web routes
  server.on("/",             handleRoot);
  server.on("/api/state",    handleState);
  server.on("/api/settings", handleSettings);
  server.on("/api/set",      handleSet);
  server.on("/api/start",    handleStart);
  server.on("/api/stop",     handleStop);
  server.on("/api/reset",    handleReset);
  server.on("/api/preset",   handlePreset);
  server.on("/api/slot",     handleSlot);
  server.on("/api/test",     handleTest);
  server.on("/api/resethigh", handleResetHigh);
  server.on("/api/factory",  handleFactory);
  server.on("/api/reboot",   handleReboot);
  server.onNotFound(handleNotFound);

  server.begin();

  settingsChanged();
  settingsDirty = false;

  generateTarget();

  oledIdle();

  soundPlay(1);
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  server.handleClient();

  soundLoop();

  checkButton();

  switch (mode) {

    case M_IDLE:     tickIdle();     break;
    case M_STARTING: tickStarting(); break;
    case M_PLAYING:  tickPlaying();  break;
    default:         tickFx();       break;   // M_FX, M_ENDING, M_TEST
  }

  // save settings shortly after the last change, never during a round
  if (settingsDirty && mode == M_IDLE && millis() - settingsDirtyAt > 1500) {

    settingsSave();

    settingsDirty = false;
  }
}
