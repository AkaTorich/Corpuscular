/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  // Common units to raw vital values (scales from synth_parameters.cpp)
  namespace ParamUnits {
    // Envelopes: value = seconds^(1/4)
    double envSeconds(double seconds);

    // Filter cutoff and effect centers are MIDI notes
    double hz(double hz);

    // LFO frequency in seconds mode is log2 of hertz
    double lfoHz(double hz);

    // Master volume: display is sqrt(v) - 80 dB
    double volumeDb(double db);

    // Oscillator levels are quadratic: amplitude = value^2
    double amplitude(double amp);

    // Synced tempo: index into kSyncedFrequencyNames
    int tempo(const std::string& name);

    // Effect chain order permutation, encoded like encodeOrderToFloat in utils.cpp
    double effectOrder(const std::vector<int>& order);
    std::vector<int> decodeEffectOrder(double value);
  }

  // Effect indices in the vital chain
  namespace Fx {
    enum { kChorus = 0, kCompressor, kDelay, kDistortion, kEq, kFilterFx, kFlanger, kPhaser, kReverb, kCount };
  }

  namespace FilterModel {
    enum { kAnalog = 0, kDirty, kLadder, kDigital, kDiode, kFormant, kComb, kPhaser };
  }

  namespace FilterStyle {
    enum { kDb12 = 0, kDb24, kNotchBlend, kNotchSpread, kBpn };
  }

  // Where an oscillator is routed
  namespace OscDest {
    enum { kFilter1 = 0, kFilter2, kBothFilters, kEffects, kDirect };
  }

  namespace OscDistortion {
    enum { kNone = 0, kSync, kFormant, kQuantize, kBend, kSqueeze, kPulseWidth,
           kFmOscA, kFmOscB, kFmSample, kRmOscA, kRmOscB, kRmSample };
  }

  namespace SpectralMorph {
    enum { kNone = 0, kVocode, kFormantScale, kHarmonicStretch, kInharmonicStretch, kSmear,
           kRandomAmplitudes, kLowPass, kHighPass, kPhaseDisperse, kShepard, kSkew };
  }

  namespace DistortionType {
    enum { kSoftClip = 0, kHardClip, kLinearFold, kSineFold, kBitCrush, kDownSample };
  }

  namespace LfoSync {
    enum { kSeconds = 0, kTempo, kDotted, kTriplet, kKeytrack };
  }

  namespace LfoTrigger {
    enum { kTrigger = 0, kSync, kEnvelope, kSustainEnvelope, kLoopPoint, kLoopHold };
  }

  namespace DelayStyle {
    enum { kMono = 0, kStereo, kPingPong, kMidPingPong };
  }

} // namespace gen
} // namespace corpuscular
