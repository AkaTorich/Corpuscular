/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  class Recipe;

  namespace RecipeCatalog {
    // All recipes in catalog order
    const std::vector<std::unique_ptr<Recipe>>& all();

    // Case-insensitive lookup by type; throws when unknown
    const Recipe& get(const std::string& type);

    // Index in the catalog, -1 when unknown
    int indexOf(const std::string& type);
  }

} // namespace gen
} // namespace corpuscular
