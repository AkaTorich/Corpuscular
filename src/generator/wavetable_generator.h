/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "json/json.h"

#include <cstdint>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  // Wavetable generator types; the first one is "Random"
  std::vector<std::string> wavetableTypes();

  struct WavetableRequest {
    std::string type = "Random";
    int frames = 64;          // 1..256
    int frame_size = 2048;    // 256..4096
    bool smooth = false;
    bool remove_dc = true;
    uint32_t seed = 0;
  };

  struct GeneratedWavetable {
    std::string name;
    std::string type;
    int frame_size = 2048;
    std::vector<std::vector<float>> frames;
  };

  // One wavetable, every frame normalized to 0.98
  GeneratedWavetable createWavetable(const WavetableRequest& request);

  enum class WavetableFormat { SerumVitalWav, Wav32, Wav16, SurgeWt, RawFloat };

  // Format labels in WavetableFormat order
  std::vector<std::string> wavetableFormatNames();

  // File extension without the dot
  std::string wavetableFormatExtension(WavetableFormat format);

  // File image of the table; SerumVitalWav resamples frames to 2048
  std::vector<uint8_t> encodeWavetable(const GeneratedWavetable& table, WavetableFormat format, const std::string& author);

  // The table as a wavetable object for an oscillator (like settings.wavetables[i] in a preset),
  // frames resampled to 2048, one keyframe per frame spread over positions 0..255
  nlohmann::json wavetableToJson(const GeneratedWavetable& table, const std::string& author);

} // namespace gen
} // namespace corpuscular
