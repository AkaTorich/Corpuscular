/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_parameters.h"

#include "../gen_random.h"

#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace corpuscular {
namespace gen {

  double ParamSpec::clamp(double value) const {
    if (!isFiniteValue(value))
      value = default_value;
    if (indexed)
      value = roundHalfEven(value);
    return value < min ? min : (value > max ? max : value);
  }

  static const std::unordered_map<std::string, const ParamSpec*>& parameterLookup() {
    static const std::unordered_map<std::string, const ParamSpec*> lookup = [] {
      std::unordered_map<std::string, const ParamSpec*> result;
      for (int i = 0; i < Parameters::count(); ++i)
        result[Parameters::at(i).name] = &Parameters::at(i);
      return result;
    }();
    return lookup;
  }

  bool Parameters::exists(const std::string& name) {
    return parameterLookup().count(name) > 0;
  }

  const ParamSpec& Parameters::get(const std::string& name) {
    const auto& lookup = parameterLookup();
    auto found = lookup.find(name);
    if (found == lookup.end())
      throw std::invalid_argument("vital 1.0.7 has no parameter '" + name + "'");
    return *found->second;
  }

} // namespace gen
} // namespace corpuscular
