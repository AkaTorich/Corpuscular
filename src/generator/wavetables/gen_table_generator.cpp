/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_table_generator.h"

#include "gen_wave_math.h"
#include "../gen_random.h"
#include "../music/gen_name_generator.h"

#include <algorithm>

namespace corpuscular {
namespace gen {

  namespace TableGenerator {
    static bool tableGeneratorEqualIgnoreCase(const std::string& a, const std::string& b) {
      if (a.size() != b.size())
        return false;
      for (size_t i = 0; i < a.size(); ++i) {
        char ca = a[i] >= 'a' && a[i] <= 'z' ? static_cast<char>(a[i] - 'a' + 'A') : a[i];
        char cb = b[i] >= 'a' && b[i] <= 'z' ? static_cast<char>(b[i] - 'a' + 'A') : b[i];
        if (ca != cb)
          return false;
      }
      return true;
    }

    const std::vector<std::unique_ptr<TableType>>& types() {
      static const std::vector<std::unique_ptr<TableType>> all = [] {
        std::vector<std::unique_ptr<TableType>> result;
        result.emplace_back(new BasicMorphType());
        result.emplace_back(new AdditiveType());
        result.emplace_back(new FmType());
        result.emplace_back(new PhaseDistortionType());
        result.emplace_back(new FormantType());
        result.emplace_back(new VocalType());
        result.emplace_back(new WavefoldType());
        result.emplace_back(new HardSyncType());
        result.emplace_back(new PwmType());
        result.emplace_back(new SpectralType());
        result.emplace_back(new ChaosType());
        result.emplace_back(new PsyBassType());
        result.emplace_back(new DigitalType());
        return result;
      }();
      return all;
    }

    std::vector<std::string> typeNames() {
      std::vector<std::string> names = { "Random" };
      for (const auto& type : types())
        names.push_back(type->name());
      return names;
    }

    static const TableType& tableGeneratorPick(const std::string& type_name, Random& random) {
      const std::vector<std::unique_ptr<TableType>>& all = types();
      for (const auto& type : all) {
        if (tableGeneratorEqualIgnoreCase(type->name(), type_name))
          return *type;
      }
      return *all[random.next(static_cast<int>(all.size()))];
    }

    TableGeneratorResult generate(const std::string& type_name, int frames, int frame_size,
                                  bool smooth, bool remove_dc, Random& random) {
      frames = std::max(kMinFrames, std::min(kMaxFrames, frames));
      frame_size = std::max(kMinFrameSize, std::min(kMaxFrameSize, frame_size));

      const TableType& type = tableGeneratorPick(type_name, random);
      TableGeneratorResult result;
      result.frames = type.generate(random, frames, frame_size);
      for (std::vector<float>& frame : result.frames) {
        if (smooth)
          WaveMath::smooth(frame, 2);
        if (remove_dc)
          WaveMath::removeDc(frame);
        WaveMath::normalize(frame);
      }
      result.name = NameGenerator::nextName(random);
      result.type = type.name();
      result.frame_size = frame_size;
      return result;
    }
  }

} // namespace gen
} // namespace corpuscular
