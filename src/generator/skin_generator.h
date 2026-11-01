/* Corpuscular skin generator
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

  // Broad music genres, one skin each
  std::vector<std::string> skinGenres();

  // Recolors `base` (a .vitalskin with vital's original multi-colour roles) into the genre's palette:
  // greys become the genre's background scale, coloured roles take the genre's hues, text keeps readable contrast,
  // corners follow the genre and "Side Panel" gets the genre's side panel colour.
  nlohmann::json makeGenreSkin(const nlohmann::json& base, const std::string& genre);

  // The 12 colour harmony rules of the skin generator on the GEN tab
  std::vector<std::string> skinHarmonies();

  // A new skin from `base`: a random key hue, the other hues by the harmony rule (index into skinHarmonies()),
  // grey scale, strength of colour, corners and side panels drawn from the seed; text keeps readable contrast
  nlohmann::json makeHarmonySkin(const nlohmann::json& base, int harmony, uint32_t seed);

} // namespace gen
} // namespace corpuscular
