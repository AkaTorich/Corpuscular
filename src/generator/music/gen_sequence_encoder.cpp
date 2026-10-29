/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_sequence_encoder.h"

#include "gen_phrase.h"
#include "gen_scales.h"
#include "../format/gen_patch.h"
#include "../format/gen_units.h"

#include <algorithm>
#include <cmath>

namespace corpuscular {
namespace gen {

  namespace SequenceEncoder {
    // Sixteen steps per cycle: "1/1" is a 1/16 step, "2/1" a 1/8 step; the cycle restarts with every note
    static void sequenceSyncLfo(Patch& patch, int lfo, const std::string& tempo) {
      std::string prefix = "lfo_" + std::to_string(lfo) + "_";
      patch[prefix + "sync"] = LfoSync::kTempo;
      patch[prefix + "tempo"] = ParamUnits::tempo(tempo);
      patch[prefix + "sync_type"] = LfoTrigger::kTrigger;
      patch[prefix + "smooth_mode"] = 1;
      patch[prefix + "smooth_time"] = -7.5;
    }

    static std::string sequenceOscPrefix(int osc) {
      return "osc_" + std::to_string(osc) + "_";
    }

    int applyPitch(Patch& patch, const Phrase& phrase, const Scale& scale, const std::vector<int>& oscs,
                   const std::string& tempo) {
      int min_note = phrase.minNote();
      int span = phrase.span();
      for (int osc : oscs) {
        std::string transpose = sequenceOscPrefix(osc) + "transpose";
        patch[transpose] = patch.get(transpose) + min_note;
        patch[sequenceOscPrefix(osc) + "transpose_quantize"] = scale.quantizeMask();
      }
      if (span == 0)
        return 0;

      int lfo = patch.takeLfo();
      std::vector<double> levels(Phrase::kSteps);
      for (int i = 0; i < Phrase::kSteps; ++i)
        levels[i] = static_cast<double>(phrase.notes[i] - min_note) / span;
      patch.lfos[lfo - 1] = LfoShape::steps("Sequence", levels);
      sequenceSyncLfo(patch, lfo, tempo);
      for (int osc : oscs)
        patch.mods.add("lfo_" + std::to_string(lfo), sequenceOscPrefix(osc) + "transpose", span / 96.0);
      return lfo;
    }

    int applyGate(Patch& patch, const Phrase& phrase, const std::vector<int>& oscs, const std::string& tempo,
                  double staccato, double decay) {
      int lfo = patch.takeLfo();
      std::vector<double> levels(Phrase::kSteps);
      std::vector<double> lengths(Phrase::kSteps);
      for (int i = 0; i < Phrase::kSteps; ++i) {
        levels[i] = std::sqrt(std::max(0.0, phrase.velocity[i]));
        lengths[i] = phrase.hold[i] >= 0.45 ? 1.0 : staccato;
      }
      patch.lfos[lfo - 1] = LfoShape::gate("Gate", levels, lengths, decay);
      sequenceSyncLfo(patch, lfo, tempo);
      for (int osc : oscs) {
        std::string level_name = sequenceOscPrefix(osc) + "level";
        double level = patch.get(level_name);
        patch[level_name] = 0;
        patch.mods.add("lfo_" + std::to_string(lfo), level_name, level);
      }
      return lfo;
    }

    int applyRollingGate(Patch& patch, const std::vector<int>& oscs, const std::vector<int>& pattern,
                         double staccato, double decay) {
      int lfo = patch.takeLfo();
      std::vector<double> levels(Phrase::kSteps);
      std::vector<double> lengths(Phrase::kSteps);
      for (int i = 0; i < Phrase::kSteps; ++i) {
        levels[i] = pattern[i % pattern.size()] == 0 ? 0.0 : 1.0;
        lengths[i] = staccato;
      }
      patch.lfos[lfo - 1] = LfoShape::gate("Rolling", levels, lengths, decay);
      sequenceSyncLfo(patch, lfo, "1/1");
      for (int osc : oscs) {
        std::string level_name = sequenceOscPrefix(osc) + "level";
        double level = patch.get(level_name);
        patch[level_name] = 0;
        patch.mods.add("lfo_" + std::to_string(lfo), level_name, level);
      }
      return lfo;
    }
  }

} // namespace gen
} // namespace corpuscular
