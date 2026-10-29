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

// --generate: builds the missing parts of the generated library in the data folder right away
// (the same library the synth builds on its first launch). Options: --presets N, --wavetables N, --lfos N, --seed N.
void runGenerateCommand(const StringArray& args);
