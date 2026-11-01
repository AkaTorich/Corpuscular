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
#include "preset_audition.h"

#include <iostream>
#include <map>

namespace {
  String generateArgument(const StringArray& args, const String& name) {
    int index = args.indexOf(name);
    if (index < 0 || index + 1 >= args.size())
      return {};
    return args[index + 1].unquoted();
  }
}

void runAuditionTest(const StringArray& args) {
  static constexpr int kTries = 8;

  String folder_name = generateArgument(args, "--audition-test");
  File folder = File::getCurrentWorkingDirectory().getChildFile(folder_name.isEmpty() ? "audition" : folder_name);
  folder.createDirectory();
  String count_setting = generateArgument(args, "--count");
  int count = count_setting.isEmpty() ? 20 : std::max(1, count_setting.getIntValue());

  PresetAudition audition;
  Random random(Time::currentTimeMillis());
  String report;
  for (const std::string& type : corpuscular::gen::presetTypes()) {
    std::map<String, int> rejected;
    int failed = 0;
    float loud_min = 0.0f, loud_max = -120.0f;
    for (int n = 0; n < count; ++n) {
      corpuscular::gen::PresetRequest request;
      request.type = type;
      request.author = "Corpuscular";
      corpuscular::gen::GeneratedPreset preset;
      PresetAudition::Result result;
      for (int attempt = 0; attempt < kTries; ++attempt) {
        request.seed = static_cast<uint32_t>(random.nextInt());
        preset = corpuscular::gen::createPreset(request);
        result = audition.check(preset.data, preset.type);
        if (result.audible)
          break;
        rejected[result.reason]++;
        // Kept for a look: what the re-made ones were
        File bad = folder.getChildFile("rejected").getChildFile(String(type))
                         .getNonexistentChildFile(String(preset.name) + " (" + result.reason + ")", ".vital", false);
        bad.getParentDirectory().createDirectory();
        bad.replaceWithText(preset.data.dump());
      }
      if (!result.audible)
        failed++;
      loud_min = std::min(loud_min, result.loud_db);
      loud_max = std::max(loud_max, result.loud_db);
      File file = folder.getChildFile(String(type)).getNonexistentChildFile(String(preset.name), ".vital", false);
      file.getParentDirectory().createDirectory();
      preset.data["preset_name"] = file.getFileNameWithoutExtension().toStdString();
      file.replaceWithText(preset.data.dump());
    }

    String line = String(type).paddedRight(' ', 11) + " made " + String(count) + ", failed " + String(failed) +
                  ", loud before level " + String(loud_min, 1) + ".." + String(loud_max, 1) + " dB, re-made:";
    for (auto& reason : rejected)
      line += " " + reason.first + " " + String(reason.second);
    report += line + "\n";
  }
  folder.getChildFile("report.txt").replaceWithText(report);
  std::cout << report << std::endl;
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
