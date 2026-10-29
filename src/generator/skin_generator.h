/* Corpuscular skin generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "json/json.h"

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

} // namespace gen
} // namespace corpuscular
