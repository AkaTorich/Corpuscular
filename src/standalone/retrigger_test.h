/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "JuceHeader.h"

// --retrigger-test [folder]: plays one note over and over like a looped DAW clip (the note-off and the next
// note-on meet at the loop point) with 1 and 8 voices, writes the WAVs into the folder (relative to the working
// folder) and prints how the sound behaves right after each loop point compared to the very first note.
void runRetriggerTest(const StringArray& args);
