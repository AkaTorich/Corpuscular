/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_recipe_catalog.h"

#include "gen_recipe.h"

#include <stdexcept>

namespace corpuscular {
namespace gen {

  namespace RecipeCatalog {
    static bool recipeCatalogEqualIgnoreCase(const std::string& a, const std::string& b) {
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

    const std::vector<std::unique_ptr<Recipe>>& all() {
      static const std::vector<std::unique_ptr<Recipe>> recipes = [] {
        std::vector<std::unique_ptr<Recipe>> result;
        result.emplace_back(new LeadRecipe());
        result.emplace_back(new BassRecipe());
        result.emplace_back(new ArpRecipe());
        result.emplace_back(new SequenceRecipe());
        result.emplace_back(new PluckRecipe());
        result.emplace_back(new StabRecipe());
        result.emplace_back(new KeysRecipe());
        result.emplace_back(new PadRecipe());
        result.emplace_back(new VocalRecipe());
        result.emplace_back(new SfxRecipe());
        result.emplace_back(new PercussionRecipe());
        return result;
      }();
      return recipes;
    }

    int indexOf(const std::string& type) {
      const std::vector<std::unique_ptr<Recipe>>& recipes = all();
      for (int i = 0; i < static_cast<int>(recipes.size()); ++i) {
        if (recipeCatalogEqualIgnoreCase(recipes[i]->type(), type))
          return i;
      }
      return -1;
    }

    const Recipe& get(const std::string& type) {
      int index = indexOf(type);
      if (index < 0)
        throw std::invalid_argument("No preset type " + type);
      return *all()[index];
    }
  }

} // namespace gen
} // namespace corpuscular
