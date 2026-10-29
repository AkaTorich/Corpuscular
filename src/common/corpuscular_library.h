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
#include "json/json.h"

// The generated library that ships with Corpuscular: on the first launch the data folder gets
// Presets/<type>, Wavetables/<type> and LFOs/<type>. A folder that already exists is never rebuilt.
namespace corpuscular {
  struct LibraryCounts {
    int presets = 512;
    int vocoder_presets = 64;   // Presets/Vocoder: presets with the vocoder switched on
    int wavetables = 256;
    int lfos = 256;
    int skins = 16;       // one per genre; more than the genres repeats nothing
  };

  // Switches the vocoder on in a generated preset with musical settings (SYNTH or TABLE carrier, style Vocal)
  void addVocoder(nlohmann::json& preset, Random& random);

  // Progress of a build: the part being made, files done and files to make over all parts
  using LibraryProgress = std::function<void(const String& part, int done, int total)>;

  // Builds the missing parts into `root` (each part is written next to its final name and renamed when done).
  // `should_stop` is polled between files. Returns false when stopped or a folder can't be written.
  bool buildLibrary(const File& root, const LibraryCounts& counts, int64 seed,
                    std::function<bool()> should_stop, LibraryProgress progress);

  // While the first-launch build runs: whether it runs, 0..1 done and a line like "Presets 120 / 512"
  bool libraryBuilding();
  float libraryBuildProgress();
  String libraryBuildStatus();

  // True when Presets, Presets/Vocoder, Wavetables, LFOs and Skins already exist in `root`
  bool libraryExists(const File& root);

  // Starts building the library in the data folder on a background thread, once per process.
  // Does nothing when the library already exists; otherwise `on_done` runs on the message thread once it is built.
  void ensureLibrary(std::function<void()> on_done);
}
