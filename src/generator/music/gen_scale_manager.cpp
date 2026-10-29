/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_scale_manager.h"

#include "../gen_random.h"

#include <algorithm>

namespace corpuscular {
namespace gen {

  // Ordinal ignore-case order (upper-cased ASCII), as StringComparer.OrdinalIgnoreCase
  static bool scaleManagerLessIgnoreCase(const std::string& a, const std::string& b) {
    size_t length = std::min(a.size(), b.size());
    for (size_t i = 0; i < length; ++i) {
      unsigned char ca = static_cast<unsigned char>(a[i]);
      unsigned char cb = static_cast<unsigned char>(b[i]);
      if (ca >= 'a' && ca <= 'z')
        ca = static_cast<unsigned char>(ca - 'a' + 'A');
      if (cb >= 'a' && cb <= 'z')
        cb = static_cast<unsigned char>(cb - 'a' + 'A');
      if (ca != cb)
        return ca < cb;
    }
    return a.size() < b.size();
  }

  static std::vector<int> scaleManagerIntervals(const ScaleDefinition& scale) {
    return std::vector<int>(scale.intervals, scale.intervals + scale.num_intervals);
  }

  std::vector<std::string> ScaleManager::getCategories() {
    std::vector<std::string> result;
    for (int i = 0; i < kNumScales; ++i) {
      std::string category = kScaleTable[i].category;
      if (std::find(result.begin(), result.end(), category) == result.end())
        result.push_back(category);
    }
    std::stable_sort(result.begin(), result.end(), scaleManagerLessIgnoreCase);
    return result;
  }

  std::vector<std::string> ScaleManager::getScalesInCategory(const std::string& category) {
    std::vector<std::string> result;
    for (int i = 0; i < kNumScales; ++i) {
      if (category == kScaleTable[i].category)
        result.push_back(kScaleTable[i].name);
    }
    std::stable_sort(result.begin(), result.end(), scaleManagerLessIgnoreCase);
    return result;
  }

  std::string ScaleManager::getRandomScaleFromCategory(const std::string& category, Random& random) {
    std::vector<std::string> scales = getScalesInCategory(category);
    if (scales.empty())
      return std::string();
    return scales[random.next(static_cast<int>(scales.size()))];
  }

  std::pair<std::string, std::string> ScaleManager::getRandomScale(Random& random) {
    std::vector<std::string> categories = getCategories();
    std::string category = categories[random.next(static_cast<int>(categories.size()))];
    std::string scale_name = getRandomScaleFromCategory(category, random);
    return { category, scale_name };
  }

  std::vector<int> ScaleManager::convertToSemitones(const std::vector<int>& intervals) {
    std::vector<int> semitones = { 0 };
    int current = 0;
    for (int interval : intervals) {
      current += interval;
      semitones.push_back(current);
    }
    return semitones;
  }

  std::vector<int> ScaleManager::getScaleAsSemitones(const std::string& category, const std::string& scale_name) {
    for (int i = 0; i < kNumScales; ++i) {
      if (category == kScaleTable[i].category && scale_name == kScaleTable[i].name)
        return convertToSemitones(scaleManagerIntervals(kScaleTable[i]));
    }
    return {};
  }

  std::vector<int> ScaleManager::getRandomScaleAsSemitones(Random& random) {
    std::pair<std::string, std::string> scale = getRandomScale(random);
    return getScaleAsSemitones(scale.first, scale.second);
  }

  std::vector<int> ScaleManager::getRandomScaleFromCategoryAsSemitones(const std::string& category, Random& random) {
    std::string scale_name = getRandomScaleFromCategory(category, random);
    if (scale_name.empty())
      return {};
    return getScaleAsSemitones(category, scale_name);
  }

  std::pair<std::string, std::string> ScaleManager::getScaleInfo(const std::vector<int>& semitones) {
    for (int i = 0; i < kNumScales; ++i) {
      if (convertToSemitones(scaleManagerIntervals(kScaleTable[i])) == semitones)
        return { kScaleTable[i].category, kScaleTable[i].name };
    }
    return { std::string(), std::string() };
  }

} // namespace gen
} // namespace corpuscular
