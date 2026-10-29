/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace corpuscular {
namespace gen {

  class Random;

  struct ScaleDefinition {
    const char* category;
    const char* name;
    int num_intervals;
    int intervals[16];
  };

  // Scale library by category; scales are stored as intervals, returned as semitones from the tonic
  class ScaleManager {
    public:
      // All categories, sorted case-insensitively
      static std::vector<std::string> getCategories();

      // Scales of one category, sorted case-insensitively
      static std::vector<std::string> getScalesInCategory(const std::string& category);

      // Empty string when the category is unknown
      static std::string getRandomScaleFromCategory(const std::string& category, Random& random);
      static std::pair<std::string, std::string> getRandomScale(Random& random);

      static std::vector<int> convertToSemitones(const std::vector<int>& intervals);

      // Empty when not found
      static std::vector<int> getScaleAsSemitones(const std::string& category, const std::string& scale_name);
      static std::vector<int> getRandomScaleAsSemitones(Random& random);
      static std::vector<int> getRandomScaleFromCategoryAsSemitones(const std::string& category, Random& random);

      // Category and name of a semitone list, empty strings when unknown
      static std::pair<std::string, std::string> getScaleInfo(const std::vector<int>& semitones);

    private:
      static const ScaleDefinition kScaleTable[];
      static const int kNumScales;
  };

} // namespace gen
} // namespace corpuscular
