/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include <string>
#include <vector>

namespace corpuscular {
namespace gen {

  class Random;

  typedef std::vector<std::vector<float>> WaveFrames;

  // A wave generator kind; generate returns frames[frame_count][frame_size], raw (not yet normalized)
  class TableType {
    public:
      virtual ~TableType() = default;
      virtual std::string name() const = 0;
      virtual WaveFrames generate(Random& random, int frames, int size) const = 0;
  };

  // Band-limited classic shapes morphing across frames
  class BasicMorphType : public TableType {
    public:
      std::string name() const override { return "Basic Morph"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Random additive spectra morphing between keyframes
  class AdditiveType : public TableType {
    public:
      std::string name() const override { return "Additive"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // FM: modulation index rises across frames
  class FmType : public TableType {
    public:
      std::string name() const override { return "FM"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Phase distortion (CZ style): warped phase read of a cosine
  class PhaseDistortionType : public TableType {
    public:
      std::string name() const override { return "Phase Distortion"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Formant: vowel-like resonances travelling through the spectrum
  class FormantType : public TableType {
    public:
      std::string name() const override { return "Formant"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Wavefolder: sine driven harder into a triangle fold each frame
  class WavefoldType : public TableType {
    public:
      std::string name() const override { return "Wavefold"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Hard sync: slave ratio sweeps upward
  class HardSyncType : public TableType {
    public:
      std::string name() const override { return "Hard Sync"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // PWM: band-limited pulse, width sweeps, blended with saw
  class PwmType : public TableType {
    public:
      std::string name() const override { return "PWM"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Spectral noise: random amplitude and phase clouds
  class SpectralType : public TableType {
    public:
      std::string name() const override { return "Spectral"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Chaos: logistic map through a one-pole lowpass
  class ChaosType : public TableType {
    public:
      std::string name() const override { return "Chaos"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Psy bass: saw/square through a resonant lowpass opening up
  class PsyBassType : public TableType {
    public:
      std::string name() const override { return "Psy Bass"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Digital: bit reduction and sample-hold on a classic shape
  class DigitalType : public TableType {
    public:
      std::string name() const override { return "Digital"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

  // Vocal: two morphing vowels with breath noise
  class VocalType : public TableType {
    public:
      std::string name() const override { return "Vocal"; }
      WaveFrames generate(Random& random, int frames, int size) const override;
  };

} // namespace gen
} // namespace corpuscular
