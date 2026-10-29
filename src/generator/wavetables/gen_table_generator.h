/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "gen_table_types.h"

#include <memory>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  class Random;

  struct TableGeneratorResult {
    std::string name;
    std::string type;
    int frame_size = 2048;
    WaveFrames frames;
  };

  // Standalone wavetables: a type (or a random one), frame count and size, smoothing, DC removal, normalization
  namespace TableGenerator {
    static constexpr int kMinFrames = 1;
    static constexpr int kMaxFrames = 256;
    static constexpr int kMinFrameSize = 256;
    static constexpr int kMaxFrameSize = 4096;

    // Generator types in the original order
    const std::vector<std::unique_ptr<TableType>>& types();

    // "Random" followed by the type names
    std::vector<std::string> typeNames();

    // An unknown type name (or "Random") picks a random type
    TableGeneratorResult generate(const std::string& type_name, int frames, int frame_size,
                                  bool smooth, bool remove_dc, Random& random);
  }

} // namespace gen
} // namespace corpuscular
