/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "preset_generator.h"

#include "gen_preset_factory.h"
#include "gen_random.h"
#include "format/gen_writer.h"
#include "music/gen_arp_algorithms.h"
#include "music/gen_lorem_generator.h"
#include "music/gen_scale_manager.h"
#include "recipes/gen_recipe.h"
#include "recipes/gen_recipe_catalog.h"

#include <set>
#include <stdexcept>

namespace corpuscular {
namespace gen {

  static std::string presetGeneratorUpper(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
      if (c >= 'a' && c <= 'z')
        c = static_cast<char>(c - 'a' + 'A');
    }
    return result;
  }

  std::vector<std::string> presetTypes() {
    std::vector<std::string> result;
    for (const auto& recipe : RecipeCatalog::all())
      result.push_back(recipe->type());
    return result;
  }

  std::vector<std::string> scaleCategories() {
    return ScaleManager::getCategories();
  }

  std::vector<std::string> scaleNames(const std::string& category) {
    return ScaleManager::getScalesInCategory(category);
  }

  std::vector<std::string> phraseKinds() {
    std::vector<std::string> result = { "random" };
    for (const std::string& kind : ArpAlgorithms::kinds())
      result.push_back(kind);
    return result;
  }

  std::string loremWords(uint32_t seed, int count) {
    Random random(seed);
    std::string result;
    for (int i = 0; i < count; ++i) {
      if (i > 0)
        result += " ";
      result += LoremGenerator::nextWord(random);
    }
    return result;
  }

  GeneratedPreset createPreset(const PresetRequest& request) {
    const Recipe& recipe = RecipeCatalog::get(request.type);
    std::optional<Scale> scale = Scale::fromLibrary(request.scale_category, request.scale_name);

    Random random(request.seed);
    std::string tag;
    std::unique_ptr<Patch> patch = PresetFactory::create(recipe.type(), random, request.author, scale,
                                                         request.phrase_kind, request.step_tempo, tag);

    GeneratedPreset result;
    result.type = recipe.type();
    result.name = PresetFactory::name(recipe.prefix(), tag, random);
    patch->name = result.name;
    result.data = PresetWriter::toJson(*patch);
    return result;
  }

  std::vector<GeneratedPreset> createBank(const BankRequest& request) {
    Random master(request.seed);
    std::vector<std::string> plan = PresetFactory::plan(request.counts, request.random_count, master);
    if (plan.empty())
      throw std::invalid_argument("No presets selected");

    std::optional<Scale> scale = Scale::fromLibrary(request.scale_category, request.scale_name);
    std::vector<uint32_t> seeds;
    for (size_t i = 0; i < plan.size(); ++i)
      seeds.push_back(static_cast<uint32_t>(master.next()));

    std::vector<GeneratedPreset> result;
    std::set<std::string> used;
    for (size_t i = 0; i < plan.size(); ++i) {
      const Recipe& recipe = RecipeCatalog::get(plan[i]);
      Random random(seeds[i]);
      std::string tag;
      std::unique_ptr<Patch> patch = PresetFactory::create(recipe.type(), random, request.author, scale,
                                                           request.phrase_kind, request.step_tempo, tag);
      std::string base_name = PresetFactory::name(recipe.prefix(), tag, random);

      // Unique names in the bank, ignoring case
      std::string name = base_name;
      while (!used.insert(presetGeneratorUpper(name)).second)
        name = base_name + " " + LoremGenerator::nextWord(master);

      patch->name = name;
      GeneratedPreset preset;
      preset.type = recipe.type();
      preset.name = name;
      preset.data = PresetWriter::toJson(*patch);
      result.push_back(std::move(preset));
    }
    return result;
  }

} // namespace gen
} // namespace corpuscular
