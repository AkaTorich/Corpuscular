/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "json/json.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  // Preset types in catalog order (Lead, Bass, Arp, Sequence, ...)
  std::vector<std::string> presetTypes();

  // Scale library categories, sorted
  std::vector<std::string> scaleCategories();

  // Scales of one category, sorted; empty for an unknown category
  std::vector<std::string> scaleNames(const std::string& category);

  // Melody algorithms for Arp and Sequence; the first one is "random"
  std::vector<std::string> phraseKinds();

  // count capitalized lorem words separated by spaces (bank names)
  std::string loremWords(uint32_t seed, int count);

  struct PresetRequest {
    std::string type;
    uint32_t seed = 0;
    std::string author;
    std::string scale_category;          // empty = psytrance scales picked at random
    std::string scale_name;
    std::string phrase_kind = "random";
    std::string step_tempo = "auto";     // "auto", "1/16" or "1/8"
  };

  struct GeneratedPreset {
    std::string type;
    std::string name;
    nlohmann::json data;                 // the full preset, as a .vital file
  };

  // One finished preset
  GeneratedPreset createPreset(const PresetRequest& request);

  struct BankRequest {
    std::map<std::string, int> counts;   // type -> number of presets
    int random_count = 0;                // plus this many presets of random types
    uint32_t seed = 0;
    std::string author;
    std::string scale_category;          // empty = psytrance scales picked at random
    std::string scale_name;
    std::string phrase_kind = "random";
    std::string step_tempo = "auto";
  };

  // A bank in catalog order with unique preset names; throws when nothing is planned
  std::vector<GeneratedPreset> createBank(const BankRequest& request);

} // namespace gen
} // namespace corpuscular
