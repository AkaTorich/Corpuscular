/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_writer.h"

#include "gen_patch.h"
#include "../gen_random.h"
#include "../wavetables/gen_wavetable_factory.h"

#include <cmath>

namespace corpuscular {
namespace gen {

  namespace PresetWriter {
    static double writerNumber(double value) {
      return isFiniteValue(value) ? value : 0.0;
    }

    static nlohmann::json writerNumberArray(const std::vector<double>& list) {
      nlohmann::json result = nlohmann::json::array();
      for (double value : list)
        result.push_back(writerNumber(value));
      return result;
    }

    static nlohmann::json writerLfos(const Patch& patch) {
      nlohmann::json result = nlohmann::json::array();
      for (const LfoShape& lfo : patch.lfos) {
        nlohmann::json shape;
        shape["name"] = lfo.name;
        shape["num_points"] = lfo.numPoints();
        shape["points"] = writerNumberArray(lfo.points);
        shape["powers"] = writerNumberArray(lfo.powers);
        shape["smooth"] = lfo.smooth;
        result.push_back(shape);
      }
      return result;
    }

    static nlohmann::json writerModulations(const Patch& patch) {
      const std::vector<ModConnection>& mods = patch.mods.connections();
      nlohmann::json result = nlohmann::json::array();
      for (int i = 0; i < Parameters::kModulationSlots; ++i) {
        nlohmann::json slot;
        bool used = i < static_cast<int>(mods.size());
        slot["destination"] = used ? mods[i].destination : std::string();
        slot["source"] = used ? mods[i].source : std::string();
        result.push_back(slot);
      }
      return result;
    }

    static nlohmann::json writerSample(const SampleData& sample) {
      nlohmann::json result;
      result["length"] = static_cast<int>(sample.data.size());
      result["name"] = sample.name;
      result["sample_rate"] = sample.sample_rate;
      result["samples"] = sample.encode();
      return result;
    }

    static nlohmann::json writerWavetable(const WavetableData& table) {
      nlohmann::json keyframes = nlohmann::json::array();
      for (const WaveKeyframe& keyframe : table.keyframes) {
        nlohmann::json frame;
        frame["position"] = keyframe.position;
        frame["wave_data"] = WavetableData::encodeWave(keyframe.wave);
        keyframes.push_back(frame);
      }

      nlohmann::json component;
      component["interpolation"] = table.interpolation;
      component["interpolation_style"] = table.interpolation_style;
      component["keyframes"] = keyframes;
      component["type"] = "Wave Source";

      nlohmann::json group;
      group["components"] = nlohmann::json::array({ component });

      nlohmann::json result;
      result["author"] = table.author;
      result["full_normalize"] = table.full_normalize;
      result["groups"] = nlohmann::json::array({ group });
      result["name"] = table.name;
      result["remove_all_dc"] = table.remove_all_dc;
      result["version"] = Parameters::kSynthVersion;
      return result;
    }

    static nlohmann::json writerSettings(const Patch& patch) {
      nlohmann::json settings;
      for (const auto& value : patch.values())
        settings[value.first] = writerNumber(value.second);

      const std::vector<ModConnection>& mods = patch.mods.connections();
      for (int i = 0; i < Parameters::kModulationSlots; ++i) {
        const ModConnection* connection = i < static_cast<int>(mods.size()) ? &mods[i] : nullptr;
        std::string prefix = "modulation_" + std::to_string(i + 1) + "_";
        settings[prefix + "amount"] = connection ? writerNumber(connection->amount) : 0.0;
        settings[prefix + "bipolar"] = connection && connection->bipolar ? 1.0 : 0.0;
        settings[prefix + "bypass"] = connection && connection->bypass ? 1.0 : 0.0;
        settings[prefix + "power"] = connection ? writerNumber(connection->power) : 0.0;
        settings[prefix + "stereo"] = connection && connection->stereo ? 1.0 : 0.0;
      }

      settings["lfos"] = writerLfos(patch);
      settings["modulations"] = writerModulations(patch);
      if (patch.sample)
        settings["sample"] = writerSample(*patch.sample);
      else
        settings["sample"] = writerSample(SampleData::whiteNoise(2048, 7));

      nlohmann::json wavetables = nlohmann::json::array();
      for (const auto& table : patch.wavetables) {
        if (table)
          wavetables.push_back(writerWavetable(*table));
        else
          wavetables.push_back(writerWavetable(WavetableFactory::init()));
      }
      settings["wavetables"] = wavetables;
      return settings;
    }

    nlohmann::json toJson(const Patch& patch) {
      nlohmann::json result;
      result["author"] = patch.author;
      result["comments"] = patch.comments;
      for (int i = 0; i < 4; ++i)
        result["macro" + std::to_string(i + 1)] = patch.macros[i];
      result["preset_name"] = patch.name;
      result["preset_style"] = patch.style;
      result["settings"] = writerSettings(patch);
      result["synth_version"] = Parameters::kSynthVersion;
      return result;
    }
  }

} // namespace gen
} // namespace corpuscular
