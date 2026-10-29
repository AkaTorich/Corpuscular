/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_wavetable_factory.h"

#include "gen_table_types.h"
#include "gen_wave_math.h"
#include "../gen_random.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace corpuscular {
namespace gen {

  namespace WavetableFactory {
    typedef std::function<std::unique_ptr<TableType>()> TableTypeMaker;

    static const std::vector<std::pair<std::string, TableTypeMaker>>& wavetableFactoryTypes() {
      static const std::vector<std::pair<std::string, TableTypeMaker>> types = {
        { "Basic Morph", [] { return std::unique_ptr<TableType>(new BasicMorphType()); } },
        { "Additive", [] { return std::unique_ptr<TableType>(new AdditiveType()); } },
        { "FM", [] { return std::unique_ptr<TableType>(new FmType()); } },
        { "Phase Distortion", [] { return std::unique_ptr<TableType>(new PhaseDistortionType()); } },
        { "Formant", [] { return std::unique_ptr<TableType>(new FormantType()); } },
        { "Wavefold", [] { return std::unique_ptr<TableType>(new WavefoldType()); } },
        { "Hard Sync", [] { return std::unique_ptr<TableType>(new HardSyncType()); } },
        { "PWM", [] { return std::unique_ptr<TableType>(new PwmType()); } },
        { "Spectral", [] { return std::unique_ptr<TableType>(new SpectralType()); } },
        { "Chaos", [] { return std::unique_ptr<TableType>(new ChaosType()); } },
        { "Psy Bass", [] { return std::unique_ptr<TableType>(new PsyBassType()); } },
        { "Digital", [] { return std::unique_ptr<TableType>(new DigitalType()); } },
        { "Vocal", [] { return std::unique_ptr<TableType>(new VocalType()); } },
      };
      return types;
    }

    static std::vector<float> wavetableFactoryFinish(std::vector<float> buffer) {
      WaveMath::removeDc(buffer);
      WaveMath::normalize(buffer, 0.98f);
      return buffer;
    }

    struct BasicWaves {
      std::vector<float> saw, sine, triangle, square;
    };

    // Computed once, thread-safe static init
    static const BasicWaves& wavetableFactoryBasicWaves() {
      static const BasicWaves waves = [] {
        const int harmonics = 300;
        const int size = WavetableData::kFrameSize;
        std::vector<double> saw(harmonics, 0.0), triangle(harmonics, 0.0), square(harmonics, 0.0), sine(harmonics, 0.0);
        sine[0] = 1.0;
        for (int n = 1; n <= harmonics; ++n) {
          saw[n - 1] = 1.0 / n;
          square[n - 1] = n % 2 == 1 ? 1.0 / n : 0.0;
          triangle[n - 1] = n % 2 == 1 ? (n % 4 == 1 ? 1 : -1) / static_cast<double>(n * n) : 0.0;
        }
        BasicWaves result;
        result.sine = wavetableFactoryFinish(WaveMath::fromHarmonics(size, sine, {}));
        result.triangle = wavetableFactoryFinish(WaveMath::fromHarmonics(size, triangle, {}));
        result.square = wavetableFactoryFinish(WaveMath::fromHarmonics(size, square, {}));
        result.saw = wavetableFactoryFinish(WaveMath::fromHarmonics(size, saw, {}));
        return result;
      }();
      return waves;
    }

    std::vector<std::string> kinds() {
      std::vector<std::string> result;
      for (const auto& type : wavetableFactoryTypes())
        result.push_back(type.first);
      return result;
    }

    WavetableData init() {
      const BasicWaves& waves = wavetableFactoryBasicWaves();
      WavetableData table;
      table.name = "Init";
      table.interpolation_style = 1;
      table.add(0, waves.saw);
      return table;
    }

    WavetableData basicShapes() {
      const BasicWaves& waves = wavetableFactoryBasicWaves();
      WavetableData table;
      table.name = "Basic Shapes";
      table.interpolation_style = 1;
      table.add(0, waves.sine);
      table.add(85, waves.triangle);
      table.add(171, waves.saw);
      table.add(256, waves.square);
      return table;
    }

    WavetableData make(const std::string& kind, Random& random, int keyframes, const std::string& name) {
      const TableTypeMaker* maker = nullptr;
      for (const auto& type : wavetableFactoryTypes()) {
        if (type.first == kind)
          maker = &type.second;
      }
      if (maker == nullptr)
        throw std::invalid_argument("No wave kind " + kind);

      keyframes = std::max(1, std::min(16, keyframes));
      WaveFrames frames = (*maker)()->generate(random, keyframes, WavetableData::kFrameSize);
      WavetableData table;
      table.name = name;
      table.interpolation_style = 1;
      for (int i = 0; i < static_cast<int>(frames.size()); ++i) {
        std::vector<float>& buffer = frames[i];
        WaveMath::removeDc(buffer);
        WaveMath::normalize(buffer, 0.98f);
        int position = keyframes == 1 ? 0 :
                       static_cast<int>(roundHalfEven(static_cast<double>(i) * WavetableData::kMaxPosition / (keyframes - 1)));
        table.add(position, std::move(buffer));
      }
      return table;
    }
  }

} // namespace gen
} // namespace corpuscular
