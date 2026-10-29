/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_patch.h"

#include <stdexcept>

namespace corpuscular {
namespace gen {

  ParamRef::operator double() const {
    return patch_.get(key_);
  }

  ParamRef& ParamRef::operator=(double value) {
    patch_.set(key_, value);
    return *this;
  }

  Patch::Patch() {
    for (int i = 0; i < Parameters::count(); ++i) {
      const ParamSpec& spec = Parameters::at(i);
      values_[spec.name] = spec.default_value;
    }
    for (LfoShape& lfo : lfos)
      lfo = LfoShape::triangle();
  }

  double Patch::get(const std::string& key) const {
    Parameters::get(key);
    return values_.at(key);
  }

  void Patch::set(const std::string& key, double value) {
    values_[key] = Parameters::get(key).clamp(value);
  }

  int Patch::takeLfo() {
    if (next_lfo_ > Parameters::kLfoCount)
      throw std::runtime_error("All 8 LFOs are taken");
    return next_lfo_++;
  }

} // namespace gen
} // namespace corpuscular
