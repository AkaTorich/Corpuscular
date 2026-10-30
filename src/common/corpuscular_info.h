/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

namespace corpuscular {
  // Product identity
  constexpr const char* kName = "Corpuscular";
  constexpr const char* kFolderName = "Corpuscular";
  constexpr const char* kVersion = "0.1.1";

  // Version written into presets, skins and wavetables. It stays at the Vital version the format comes from,
  // so files open both in Corpuscular and in Vital 1.0.x (Vital rejects files from a "newer" major.minor).
  constexpr const char* kFormatVersion = "1.0.7";
  constexpr int kFormatVersionNumber = 0x10007;

  // Where Vital keeps its library, so Corpuscular can show it next to its own
  constexpr const char* kVitalFolderName = "Vital";
}
