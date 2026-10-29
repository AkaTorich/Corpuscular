/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "corpuscular_library.h"

#include "lfo_generator.h"
#include "load_save.h"
#include "preset_generator.h"
#include "qv_wavetable.h"
#include "skin_generator.h"
#include "synth_constants.h"
#include "wavetable_generator.h"

#include <atomic>

namespace corpuscular {
  namespace {
    const char* kLibraryAuthor = "Corpuscular";
    const char* kLibraryPartSuffix = ".generating";
    const char* kVocoderFolderName = "Vocoder";
    constexpr int kLibraryTableFrames = 64;
    constexpr int kLibraryTableFrameSize = 2048;

    String librarySafeName(const String& name, const String& fallback) {
      String result = File::createLegalFileName(name.trim()).trim();
      return result.isEmpty() ? fallback : result;
    }

    // Spreads `total` items over `num_kinds` kinds as evenly as possible
    std::vector<int> libraryEvenCounts(int total, int num_kinds) {
      std::vector<int> counts(num_kinds, num_kinds ? total / num_kinds : 0);
      for (int i = 0; num_kinds && i < total % num_kinds; ++i)
        counts[i]++;
      return counts;
    }

    File libraryUniqueFile(const File& folder, const String& name, const String& extension) {
      String base = librarySafeName(name, "Untitled");
      String candidate = base;
      for (int n = 2; folder.getChildFile(candidate + extension).exists(); ++n)
        candidate = base + " " + String(n);
      return folder.getChildFile(candidate + extension);
    }
  }

  void addVocoder(json& preset, Random& random) {
    static const int kBands[] = { 12, 16, 20, 24, 32 };
    auto between = [&random](float low, float high) { return low + (high - low) * random.nextFloat(); };

    json& settings = preset["settings"];
    bool table = random.nextBool();
    float pitch_roll = random.nextFloat();
    settings["vocoder_on"] = 1.0f;
    settings["vocoder_carrier"] = table ? 1.0f : 0.0f;
    settings["vocoder_pitch_mode"] = pitch_roll < 0.5f ? 0.0f : (pitch_roll < 0.85f ? 2.0f : 1.0f);
    settings["vocoder_note"] = (float)(36 + random.nextInt(37));
    settings["vocoder_octave"] = (float)(random.nextInt(3) - 1);
    settings["vocoder_position"] = between(0.0f, 1.0f);
    settings["vocoder_unison"] = (float)(1 + random.nextInt(5));
    settings["vocoder_detune"] = between(5.0f, 30.0f);
    settings["vocoder_glide"] = between(0.0f, 120.0f);
    settings["vocoder_bands"] = (float)kBands[random.nextInt(5)];
    settings["vocoder_width"] = between(0.6f, 1.8f);
    settings["vocoder_formant"] = between(-0.4f, 0.4f);
    settings["vocoder_attack"] = between(1.0f, 15.0f);
    settings["vocoder_release"] = between(30.0f, 250.0f);
    settings["vocoder_air"] = between(0.1f, 0.5f);
    settings["vocoder_noise"] = between(0.3f, 0.8f);
    settings["vocoder_gate"] = between(-80.0f, -55.0f);
    settings["vocoder_tilt"] = between(-0.5f, 0.5f);
    settings["vocoder_spread"] = between(0.0f, 0.8f);
    settings["vocoder_freeze"] = 0.0f;
    settings["vocoder_dry_wet"] = between(0.7f, 1.0f);
    settings["vocoder_level"] = 0.0f;

    preset["corpuscular_vocoder"] = { { "slot", (int)vital::constants::kVocoder },
                                      { "table_builtin", random.nextInt(wv::Wavetable::numBuiltIns()) } };
    preset["preset_style"] = "Vocal";
  }

  namespace {
    // One part of the library (Presets, LFOs or Wavetables) with a subfolder per kind
    struct LibraryPart {
      String folder_name;
      String label;
      std::vector<std::string> kinds;
      int total = 0;
      std::function<bool(const std::string&, const File&)> make;
      bool flat = false;    // all files straight in the part folder, no folder per kind
    };

    bool libraryPartMissing(const File& root, const LibraryPart& part) {
      return part.total > 0 && !root.getChildFile(part.folder_name).exists();
    }

    bool libraryBuildPart(const File& root, const LibraryPart& part, int& done, int all,
                          std::function<bool()> should_stop, LibraryProgress progress) {
      File part_folder = root.getChildFile(part.folder_name + kLibraryPartSuffix);
      if (part_folder.exists())
        part_folder.deleteRecursively();
      if (!part_folder.createDirectory())
        return false;

      std::vector<int> counts = libraryEvenCounts(part.total, (int)part.kinds.size());
      int written = 0;
      for (size_t k = 0; k < part.kinds.size(); ++k) {
        File kind_folder = part.flat ? part_folder : part_folder.getChildFile(librarySafeName(part.kinds[k], part.folder_name));
        if (counts[k] > 0 && !kind_folder.createDirectory())
          return false;

        for (int n = 0; n < counts[k]; ++n) {
          if (should_stop && should_stop())
            return false;
          part.make(part.kinds[k], kind_folder);
          written++;
          done++;
          if (progress)
            progress(part.label + " " + String(written) + " / " + String(part.total), done, all);
        }
      }

      return part_folder.moveFileTo(root.getChildFile(part.folder_name));
    }

    class LibraryBuilder : public Thread {
      public:
        LibraryBuilder() : Thread("Corpuscular library") { }
        ~LibraryBuilder() { stopThread(20000); }

        void addCallback(std::function<void()> callback) {
          ScopedLock lock(lock_);
          if (done_)
            MessageManager::callAsync(callback);
          else
            callbacks_.push_back(std::move(callback));
        }

        bool building() const { return !done_flag_.load(); }
        float progress() const { return progress_.load(); }
        String status() {
          ScopedLock lock(lock_);
          return status_;
        }

        void run() override {
          buildLibrary(LoadSave::getDataDirectory(), LibraryCounts(), Time::currentTimeMillis(),
                       [this]() { return threadShouldExit(); },
                       [this](const String& part, int done, int total) {
                         progress_.store(total > 0 ? done / (float)total : 1.0f);
                         ScopedLock lock(lock_);
                         status_ = part;
                       });
          done_flag_.store(true);
          if (threadShouldExit())
            return;

          std::vector<std::function<void()>> callbacks;
          {
            ScopedLock lock(lock_);
            done_ = true;
            callbacks.swap(callbacks_);
          }
          for (auto& callback : callbacks)
            MessageManager::callAsync(callback);
        }

      private:
        CriticalSection lock_;
        bool done_ = false;
        std::atomic<bool> done_flag_ { false };
        std::atomic<float> progress_ { 0.0f };
        String status_;
        std::vector<std::function<void()>> callbacks_;
    };

    std::unique_ptr<LibraryBuilder>& libraryBuilder() {
      static std::unique_ptr<LibraryBuilder> builder;
      return builder;
    }

    // Signature skins made earlier for the preset generator, shipped as they are
    void installSignatureSkins(const File& root) {
      struct SignatureSkin { const char* name; const char* data; int size; };
      const SignatureSkin skins[] = {
        { "Psy Night Stage", BinaryData::psy_night_stage_vitalskin, BinaryData::psy_night_stage_vitalskinSize },
        { "Psy Day Stage", BinaryData::psy_day_stage_vitalskin, BinaryData::psy_day_stage_vitalskinSize },
        { "Neon Tubes", BinaryData::neon_tubes_vitalskin, BinaryData::neon_tubes_vitalskinSize },
      };

      File folder = root.getChildFile(LoadSave::kSkinFolderName);
      if (!folder.isDirectory())
        return;
      for (const SignatureSkin& skin : skins) {
        File file = folder.getChildFile(String(skin.name) + "." + vital::kSkinExtension);
        if (!file.exists())
          file.replaceWithData(skin.data, skin.size);
      }
    }
  }

  bool libraryExists(const File& root) {
    return root.getChildFile(LoadSave::kPresetFolderName).isDirectory() &&
           root.getChildFile(LoadSave::kPresetFolderName).getChildFile(kVocoderFolderName).isDirectory() &&
           root.getChildFile(LoadSave::kWavetableFolderName).isDirectory() &&
           root.getChildFile(LoadSave::kLfoFolderName).isDirectory() &&
           root.getChildFile(LoadSave::kSkinFolderName).isDirectory();
  }

  bool buildLibrary(const File& root, const LibraryCounts& counts, int64 seed,
                    std::function<bool()> should_stop, LibraryProgress progress) {
    if (!root.isDirectory() && !root.createDirectory())
      return false;

    Random random(seed);

    // Wavetable kinds are the generator types without "Random"
    std::vector<std::string> table_types = gen::wavetableTypes();
    if (!table_types.empty())
      table_types.erase(table_types.begin());

    std::vector<LibraryPart> parts;
    parts.push_back({ LoadSave::kPresetFolderName, "Presets", gen::presetTypes(), counts.presets,
                      [&](const std::string& type, const File& folder) {
      gen::PresetRequest request;
      request.type = type;
      request.seed = static_cast<uint32_t>(random.nextInt());
      request.author = kLibraryAuthor;
      gen::GeneratedPreset preset = gen::createPreset(request);

      File file = libraryUniqueFile(folder, preset.name, String(".") + vital::kPresetExtension);
      preset.data["preset_name"] = file.getFileNameWithoutExtension().toStdString();
      return file.replaceWithText(preset.data.dump());
    } });

    // A share of presets with the vocoder on: the preset's own sound is the voice until a real one comes in
    LibraryPart vocoder_presets { String(LoadSave::kPresetFolderName) + "/" + kVocoderFolderName, "Vocoder presets",
                                  gen::presetTypes(), counts.vocoder_presets,
                                  [&](const std::string& type, const File& folder) {
      gen::PresetRequest request;
      request.type = type;
      request.seed = static_cast<uint32_t>(random.nextInt());
      request.author = kLibraryAuthor;
      gen::GeneratedPreset preset = gen::createPreset(request);
      addVocoder(preset.data, random);

      File file = libraryUniqueFile(folder, preset.name, String(".") + vital::kPresetExtension);
      preset.data["preset_name"] = file.getFileNameWithoutExtension().toStdString();
      return file.replaceWithText(preset.data.dump());
    } };
    vocoder_presets.flat = true;
    parts.push_back(vocoder_presets);

    parts.push_back({ LoadSave::kLfoFolderName, "LFOs", gen::lfoTypes(), counts.lfos,
                      [&](const std::string& type, const File& folder) {
      gen::GeneratedLfo lfo = gen::createLfo(type, static_cast<uint32_t>(random.nextInt()));
      File file = libraryUniqueFile(folder, lfo.name, String(".") + vital::kLfoExtension);
      lfo.data["name"] = file.getFileNameWithoutExtension().toStdString();
      return file.replaceWithText(lfo.data.dump());
    } });

    parts.push_back({ LoadSave::kWavetableFolderName, "Wavetables", table_types, counts.wavetables,
                      [&](const std::string& type, const File& folder) {
      gen::WavetableRequest request;
      request.type = type;
      request.frames = kLibraryTableFrames;
      request.frame_size = kLibraryTableFrameSize;
      request.seed = static_cast<uint32_t>(random.nextInt());
      gen::GeneratedWavetable table = gen::createWavetable(request);

      File file = libraryUniqueFile(folder, table.name, String(".") + vital::kWavetableExtension);
      table.name = file.getFileNameWithoutExtension().toStdString();
      return file.replaceWithText(gen::wavetableToJson(table, kLibraryAuthor).dump());
    } });

    // Skins: one per broad genre, recoloured from vital's original multi-colour skin
    json base_skin = json::parse(std::string(BinaryData::vital_original_vitalskin, BinaryData::vital_original_vitalskinSize),
                                 nullptr, false);
    std::vector<std::string> genres = gen::skinGenres();
    LibraryPart skins { LoadSave::kSkinFolderName, "Skins", genres, base_skin.is_object() ? counts.skins : 0,
                        [&](const std::string& genre, const File& folder) {
      File file = folder.getChildFile(librarySafeName(genre, "Skin") + "." + vital::kSkinExtension);
      return file.replaceWithText(gen::makeGenreSkin(base_skin, genre).dump());
    } };
    skins.flat = true;
    parts.push_back(skins);

    int all = 0;
    for (const LibraryPart& part : parts) {
      if (libraryPartMissing(root, part))
        all += part.total;
    }

    int done = 0;
    for (const LibraryPart& part : parts) {
      if (libraryPartMissing(root, part) && !libraryBuildPart(root, part, done, all, should_stop, progress))
        return false;
    }
    installSignatureSkins(root);
    return true;
  }

  bool libraryBuilding() {
    std::unique_ptr<LibraryBuilder>& builder = libraryBuilder();
    return builder != nullptr && builder->building();
  }

  float libraryBuildProgress() {
    std::unique_ptr<LibraryBuilder>& builder = libraryBuilder();
    return builder ? builder->progress() : 1.0f;
  }

  String libraryBuildStatus() {
    std::unique_ptr<LibraryBuilder>& builder = libraryBuilder();
    return builder ? builder->status() : String();
  }

  void ensureLibrary(std::function<void()> on_done) {
    if (libraryExists(LoadSave::getDataDirectory())) {
      installSignatureSkins(LoadSave::getDataDirectory());
      return;
    }

    std::unique_ptr<LibraryBuilder>& builder = libraryBuilder();
    if (builder == nullptr) {
      builder = std::make_unique<LibraryBuilder>();
      builder->startThread(1);
    }
    if (on_done)
      builder->addCallback(std::move(on_done));
  }
}
