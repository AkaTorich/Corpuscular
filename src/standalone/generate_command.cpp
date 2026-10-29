/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "generate_command.h"

#include "corpuscular_library.h"
#include "load_save.h"

#include <iostream>

namespace {
  String generateArgument(const StringArray& args, const String& name) {
    int index = args.indexOf(name);
    if (index < 0 || index + 1 >= args.size())
      return {};
    return args[index + 1].unquoted();
  }
}

void runGenerateCommand(const StringArray& args) {
  corpuscular::LibraryCounts counts;
  String presets = generateArgument(args, "--presets");
  String wavetables = generateArgument(args, "--wavetables");
  String lfos = generateArgument(args, "--lfos");
  if (presets.isNotEmpty())
    counts.presets = std::max(0, presets.getIntValue());
  if (wavetables.isNotEmpty())
    counts.wavetables = std::max(0, wavetables.getIntValue());
  if (lfos.isNotEmpty())
    counts.lfos = std::max(0, lfos.getIntValue());

  String seed = generateArgument(args, "--seed");
  File root = LoadSave::getDataDirectory();
  bool done = corpuscular::buildLibrary(root, counts, seed.isEmpty() ? Time::currentTimeMillis() : seed.getLargeIntValue(),
                                        nullptr, [](const String& part, int done, int total) {
                                          if (done % 32 == 0 || done == total)
                                            std::cout << part << std::endl;
                                        });
  std::cout << (done ? "library ready: " : "library failed: ") << root.getFullPathName() << std::endl;
}
