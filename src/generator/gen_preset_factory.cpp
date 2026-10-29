/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_preset_factory.h"

#include "gen_random.h"
#include "gen_sanity.h"
#include "music/gen_arp_algorithms.h"
#include "music/gen_lorem_generator.h"
#include "recipes/gen_kit.h"
#include "recipes/gen_recipe.h"
#include "recipes/gen_recipe_catalog.h"

#include <algorithm>

namespace corpuscular {
namespace gen {

  namespace PresetFactory {
    static std::string presetFactoryCapital(std::string word) {
      if (!word.empty() && word[0] >= 'a' && word[0] <= 'z')
        word[0] = static_cast<char>(word[0] - 'a' + 'A');
      return word;
    }

    std::unique_ptr<Patch> create(const std::string& type, Random& random, const std::string& author,
                                  const std::optional<Scale>& fixed_scale, const std::string& phrase_kind,
                                  const std::string& step_tempo, std::string& tag) {
      const Recipe& recipe = RecipeCatalog::get(type);
      std::unique_ptr<Patch> patch(new Patch());
      patch->style = recipe.style();
      patch->author = author;

      Scale scale = fixed_scale ? *fixed_scale : Scale::randomPsy(random);
      ArpAlgorithms arp(random);
      RecipeContext context(*patch, random, scale, arp);
      context.phrase_kind = phrase_kind;
      context.step_tempo = step_tempo;

      recipe.build(context);
      Kit::psyChain(context);
      Sanity::apply(*patch, recipe.type());
      tag = context.tag;
      return patch;
    }

    std::vector<std::string> plan(const std::map<std::string, int>& counts, int random_count, Random& random) {
      const std::vector<std::unique_ptr<Recipe>>& recipes = RecipeCatalog::all();
      std::vector<std::string> result;
      for (const auto& recipe : recipes) {
        auto found = counts.find(recipe->type());
        if (found != counts.end()) {
          for (int i = 0; i < found->second; ++i)
            result.push_back(recipe->type());
        }
      }
      for (int i = 0; i < random_count; ++i)
        result.push_back(recipes[random.next(static_cast<int>(recipes.size()))]->type());

      std::stable_sort(result.begin(), result.end(), [](const std::string& a, const std::string& b) {
        return RecipeCatalog::indexOf(a) < RecipeCatalog::indexOf(b);
      });
      return result;
    }

    std::string name(const std::string& prefix, const std::string& /* tag */, Random& random) {
      std::string word = presetFactoryCapital(LoremGenerator::nextWord(random));
      if (random.nextDouble() < 0.35)
        word += " " + presetFactoryCapital(LoremGenerator::nextWord(random));
      return prefix + " " + word;
    }
  }

} // namespace gen
} // namespace corpuscular
