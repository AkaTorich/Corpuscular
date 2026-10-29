/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>

namespace corpuscular {
namespace gen {

  class Patch;

  // Last check: the preset must sound and must not overload the output
  namespace Sanity {
    void apply(Patch& patch, const std::string& type);
  }

} // namespace gen
} // namespace corpuscular
