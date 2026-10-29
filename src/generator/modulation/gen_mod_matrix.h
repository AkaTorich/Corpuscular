/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  struct ModConnection {
    std::string source;
    std::string destination;
    double amount = 0.0;
    double power = 0.0;
    bool bipolar = false;
    bool stereo = false;
    bool bypass = false;
  };

  // Modulation matrix: 64 slots; amount is a fraction of the destination's full range
  class ModMatrix {
    public:
      const std::vector<ModConnection>& connections() const { return connections_; }

      int freeSlots() const;

      static bool isSource(const std::string& name);

      bool add(const std::string& source, const std::string& destination, double amount,
               bool bipolar = false, double power = 0.0, bool stereo = false);

    private:
      std::vector<ModConnection> connections_;
  };

} // namespace gen
} // namespace corpuscular
