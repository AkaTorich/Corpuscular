/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "format/gen_patch.h"
#include "music/gen_scales.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  class Random;

  // Builds presets: type plan -> presets -> unique names
  namespace PresetFactory {
    // One preset of the type; fixed_scale empty = a random psytrance scale; tag receives the name suffix
    std::unique_ptr<Patch> create(const std::string& type, Random& random, const std::string& author,
                                  const std::optional<Scale>& fixed_scale, const std::string& phrase_kind,
                                  const std::string& step_tempo, std::string& tag);

    // Types to make, sorted in catalog order: counts per type plus random_count random types
    std::vector<std::string> plan(const std::map<std::string, int>& counts, int random_count, Random& random);

    // Preset name: type prefix and one or two lorem words
    std::string name(const std::string& prefix, const std::string& tag, Random& random);
  }

} // namespace gen
} // namespace corpuscular
