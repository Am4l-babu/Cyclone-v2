/*
   ============================================================
     BUZZER TEST  -  Cyclone Target Lock
   ============================================================

   Plays a scale, then every game sound effect in turn so you can
   check the buzzer wiring and pick your favourites.
   Open the Serial Monitor at 115200 baud.

   WIRING
   ------------------------------------------------------------
   BUZZER (+)  -> D7 / GPIO13
   BUZZER (-)  -> GND
   Use a PASSIVE buzzer for real melodies. An ACTIVE buzzer only
   makes one fixed tone, whatever frequency is requested.

   EXPECTED RESULT
   ------------------------------------------------------------
   1. Eight ascending notes (C major scale)
   2. Each game sound, named on the Serial Monitor:
      hit, miss, level up, start, game over, win ...
   3. Repeats every few seconds
*/

#include <Arduino.h>

#define BUZZER_PIN  D7

struct Note {
  uint16_t freq;
  uint16_t ms;
};

const Note SCALE[] = {
  {523, 150}, {587, 150}, {659, 150}, {698, 150},
  {784, 150}, {880, 150}, {988, 150}, {1047, 300}
};

const Note CHIRP_UP[] = { {800, 40}, {1100, 40}, {1500, 50} };
const Note CHIRP_DN[] = { {1400, 45}, {1000, 45}, {650, 60} };
const Note COIN[]     = { {988, 70}, {1319, 220} };
const Note BUZZ[]     = { {160, 120}, {0, 30}, {160, 120} };
const Note FANFARE[]  = { {523, 110}, {0, 25}, {523, 110}, {0, 25}, {523, 110},
                          {0, 25}, {659, 180}, {784, 140}, {1047, 450} };
const Note SAD[]      = { {392, 250}, {370, 250}, {349, 250}, {330, 600} };
const Note SIREN[]    = { {900, 150}, {1200, 150}, {900, 150},
                          {1200, 150}, {900, 150}, {1200, 150} };
const Note POWERUP[]  = { {523, 60}, {659, 60}, {784, 60}, {1047, 60}, {1319, 60}, {1568, 160} };
const Note LASER[]    = { {2400, 15}, {2000, 15}, {1600, 15}, {1300, 15},
                          {1000, 15}, {800, 15}, {600, 15}, {450, 20} };

void play(const char* name, const Note* notes, size_t count) {

  Serial.print("  ");
  Serial.println(name);

  for (size_t i = 0; i < count; i++) {

    if (notes[i].freq)
      tone(BUZZER_PIN, notes[i].freq);
    else
      noTone(BUZZER_PIN);

    delay(notes[i].ms);
  }

  noTone(BUZZER_PIN);

  delay(500);
}

#define PLAY(name, arr)  play(name, arr, sizeof(arr) / sizeof(arr[0]))

void setup() {

  Serial.begin(115200);

  delay(500);

  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println();
  Serial.println("==============================");
  Serial.println(" BUZZER TEST  (pin D7 / GPIO13)");
  Serial.println("==============================");
}

void loop() {

  PLAY("Scale", SCALE);

  delay(500);

  PLAY("Chirp up   (hit)",         CHIRP_UP);
  PLAY("Chirp down",               CHIRP_DN);
  PLAY("Coin       (level up)",    COIN);
  PLAY("Low buzz   (miss)",        BUZZ);
  PLAY("Power up   (round start)", POWERUP);
  PLAY("Laser",                    LASER);
  PLAY("Siren",                    SIREN);
  PLAY("Sad trombone (game over)", SAD);
  PLAY("Fanfare    (win)",         FANFARE);

  Serial.println("--- loop complete, restarting ---");

  delay(3000);
}
