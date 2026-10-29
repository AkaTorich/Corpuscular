/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "gen_lfo_shape.h"
#include "gen_parameters.h"
#include "gen_sample.h"
#include "gen_wavetable.h"
#include "../modulation/gen_mod_matrix.h"

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

namespace corpuscular {
namespace gen {

  class Patch;

  // Reads and clamped writes of one parameter: patch["osc_1_level"] = 0.7;
  class ParamRef {
    public:
      ParamRef(Patch& patch, std::string key) : patch_(patch), key_(std::move(key)) { }

      operator double() const;
      ParamRef& operator=(double value);
      ParamRef& operator=(const ParamRef& other) { return *this = static_cast<double>(other); }

    private:
      Patch& patch_;
      std::string key_;
  };

  // One vital preset: every parameter starts at its default, writes only to existing keys
  class Patch {
    public:
      Patch();

      ParamRef operator[](const std::string& key) { return ParamRef(*this, key); }
      double get(const std::string& key) const;
      void set(const std::string& key, double value);

      const std::unordered_map<std::string, double>& values() const { return values_; }

      // Free LFO (1..8), each recipe step takes one
      int takeLfo();
      bool hasFreeLfo() const { return next_lfo_ <= Parameters::kLfoCount; }

      std::string name;
      std::string style = "Lead";
      std::string author;
      std::string comments;
      std::array<std::string, 4> macros = { { "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4" } };

      ModMatrix mods;
      std::array<LfoShape, Parameters::kLfoCount> lfos;
      std::array<std::unique_ptr<WavetableData>, Parameters::kOscCount> wavetables;
      std::unique_ptr<SampleData> sample;

    private:
      std::unordered_map<std::string, double> values_;
      int next_lfo_ = 1;
  };

} // namespace gen
} // namespace corpuscular
