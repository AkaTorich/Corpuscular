/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "gen_mod_matrix.h"

#include "../format/gen_parameters.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace corpuscular {
namespace gen {

  static const std::unordered_set<std::string>& modMatrixSources() {
    static const std::unordered_set<std::string> sources = [] {
      std::unordered_set<std::string> result = {
        "random", "stereo", "note", "note_in_octave", "aftertouch", "velocity",
        "slide", "lift", "mod_wheel", "pitch_wheel"
      };
      for (int i = 1; i <= 6; ++i)
        result.insert("env_" + std::to_string(i));
      for (int i = 1; i <= Parameters::kLfoCount; ++i)
        result.insert("lfo_" + std::to_string(i));
      for (int i = 1; i <= 4; ++i)
        result.insert("random_" + std::to_string(i));
      for (int i = 1; i <= 4; ++i)
        result.insert("macro_control_" + std::to_string(i));
      return result;
    }();
    return sources;
  }

  int ModMatrix::freeSlots() const {
    return Parameters::kModulationSlots - static_cast<int>(connections_.size());
  }

  bool ModMatrix::isSource(const std::string& name) {
    return modMatrixSources().count(name) > 0;
  }

  bool ModMatrix::add(const std::string& source, const std::string& destination, double amount,
                      bool bipolar, double power, bool stereo) {
    if (!isSource(source))
      throw std::invalid_argument("No modulation source '" + source + "'");
    Parameters::get(destination);
    if (static_cast<int>(connections_.size()) >= Parameters::kModulationSlots)
      return false;
    for (const ModConnection& connection : connections_) {
      if (connection.source == source && connection.destination == destination)
        return false;
    }

    ModConnection connection;
    connection.source = source;
    connection.destination = destination;
    connection.amount = std::max(-1.0, std::min(1.0, amount));
    connection.power = std::max(-10.0, std::min(10.0, power));
    connection.bipolar = bipolar;
    connection.stereo = stereo;
    connections_.push_back(connection);
    return true;
  }

} // namespace gen
} // namespace corpuscular
