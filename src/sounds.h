// Cyclone Target Lock - buzzer sound effects (non-blocking note sequencer)
#pragma once

#include <Arduino.h>

#define BUZZER_PIN   D7        // GPIO13, passive buzzer recommended
#define SOUND_COUNT  12

const char* soundName(uint8_t id);

void soundBegin();

// Start a sound. Silent when the buzzer is disabled in the settings,
// unless force is true (used by the web "preview" buttons).
void soundPlay(uint8_t id, bool force = false);

void soundStop();

bool soundBusy();

// Call from loop(); advances the note sequence without blocking.
void soundLoop();
