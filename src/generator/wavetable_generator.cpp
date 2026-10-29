/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "wavetable_generator.h"

#include "gen_random.h"
#include "format/gen_parameters.h"
#include "format/gen_wavetable.h"
#include "wavetables/gen_table_generator.h"
#include "wavetables/gen_wavetable_encoders.h"

namespace corpuscular {
namespace gen {

  std::vector<std::string> wavetableTypes() {
    return TableGenerator::typeNames();
  }

  GeneratedWavetable createWavetable(const WavetableRequest& request) {
    Random random(request.seed);
    TableGeneratorResult made = TableGenerator::generate(request.type, request.frames, request.frame_size,
                                                         request.smooth, request.remove_dc, random);
    GeneratedWavetable result;
    result.name = made.name;
    result.type = made.type;
    result.frame_size = made.frame_size;
    result.frames = std::move(made.frames);
    return result;
  }

  std::vector<std::string> wavetableFormatNames() {
    return { "Wavetable WAV 2048 (.wav)", "WAV 32-bit (.wav)", "WAV 16-bit (.wav)", "Wavetable .wt (.wt)", "Raw float (.raw)" };
  }

  std::string wavetableFormatExtension(WavetableFormat format) {
    switch (format) {
      case WavetableFormat::SurgeWt: return "wt";
      case WavetableFormat::RawFloat: return "raw";
      default: return "wav";
    }
  }

  std::vector<uint8_t> encodeWavetable(const GeneratedWavetable& table, WavetableFormat format, const std::string& author) {
    switch (format) {
      case WavetableFormat::SerumVitalWav: {
        // Serum and Vital read 2048-sample frames
        const int kSerumFrameSize = WavetableData::kFrameSize;
        if (table.frame_size == kSerumFrameSize)
          return WavetableEncoders::wav(table.frames, table.frame_size, true, true, author, table.name, table.type);
        std::vector<std::vector<float>> frames;
        for (const std::vector<float>& frame : table.frames)
          frames.push_back(WavetableEncoders::resampleFrame(frame, kSerumFrameSize));
        return WavetableEncoders::wav(frames, kSerumFrameSize, true, true, author, table.name, table.type);
      }
      case WavetableFormat::Wav32:
        return WavetableEncoders::wav(table.frames, table.frame_size, true, true, author, table.name, table.type);
      case WavetableFormat::Wav16:
        return WavetableEncoders::wav(table.frames, table.frame_size, false, true, author, table.name, table.type);
      case WavetableFormat::SurgeWt:
        return WavetableEncoders::wt(table.frames, table.frame_size);
      default:
        return WavetableEncoders::raw(table.frames, table.frame_size);
    }
  }

  nlohmann::json wavetableToJson(const GeneratedWavetable& table, const std::string& author) {
    const int kLastPosition = 255;
    int count = static_cast<int>(table.frames.size());

    nlohmann::json keyframes = nlohmann::json::array();
    for (int i = 0; i < count; ++i) {
      int position = count <= 1 ? 0 : (i * kLastPosition + (count - 1) / 2) / (count - 1);
      std::vector<float> wave = WavetableEncoders::resampleFrame(table.frames[i], WavetableData::kFrameSize);
      nlohmann::json frame;
      frame["position"] = position;
      frame["wave_data"] = WavetableData::encodeWave(wave);
      keyframes.push_back(frame);
    }

    nlohmann::json component;
    component["interpolation"] = 0;
    component["interpolation_style"] = 1;
    component["keyframes"] = keyframes;
    component["type"] = "Wave Source";

    nlohmann::json group;
    group["components"] = nlohmann::json::array({ component });

    nlohmann::json result;
    result["author"] = author;
    result["full_normalize"] = true;
    result["groups"] = nlohmann::json::array({ group });
    result["name"] = table.name;
    result["remove_all_dc"] = true;
    result["version"] = Parameters::kSynthVersion;
    return result;
  }

} // namespace gen
} // namespace corpuscular
