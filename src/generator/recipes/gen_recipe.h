/* Corpuscular preset generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#pragma once

#include "../gen_random.h"
#include "../music/gen_scales.h"

#include <initializer_list>
#include <string>

namespace corpuscular {
namespace gen {

  class ArpAlgorithms;
  class Patch;

  struct RecipeContext {
    RecipeContext(Patch& patch, Random& random, Scale preset_scale, ArpAlgorithms& arp_algorithms) :
        p(patch), r(random), scale(std::move(preset_scale)), arp(arp_algorithms) { }

    double range(double a, double b) { return a + r.nextDouble() * (b - a); }
    int rangeInt(int a, int b) { return r.next(a, b + 1); }
    bool chance(double probability) { return r.nextDouble() < probability; }

    template <typename T>
    T pick(std::initializer_list<T> items) { return items.begin()[r.next(static_cast<int>(items.size()))]; }

    std::string pick(std::initializer_list<const char*> items) {
      return items.begin()[r.next(static_cast<int>(items.size()))];
    }

    // Sequence step: "1/1" is 16 steps per bar (sixteenths), "2/1" is eighths
    std::string sequenceTempo() {
      if (step_tempo == "1/16")
        return "1/1";
      if (step_tempo == "1/8")
        return "2/1";
      return chance(0.8) ? "1/1" : "2/1";
    }

    Patch& p;
    Random& r;
    Scale scale;
    ArpAlgorithms& arp;
    std::string phrase_kind = "random";   // melody algorithm for Arp and Sequence
    std::string step_tempo = "auto";      // "1/16", "1/8" or "auto"
    std::string tag;                      // name suffix ("Rolling", "Kick"...)
  };

  class Recipe {
    public:
      virtual ~Recipe() = default;
      virtual std::string type() const = 0;     // type name in the menu and bank folder
      virtual std::string style() const = 0;    // preset style
      virtual std::string prefix() const = 0;   // preset name prefix
      virtual void build(RecipeContext& c) const = 0;
  };

  // Lead under a DAW arp: bright, with delay and moving timbre
  class LeadRecipe : public Recipe {
    public:
      std::string type() const override { return "Lead"; }
      std::string style() const override { return "Lead"; }
      std::string prefix() const override { return "LD"; }
      void build(RecipeContext& c) const override;
  };

  // Psytrance bass: mono, short filter click, dry; half of them roll inside the preset
  class BassRecipe : public Recipe {
    public:
      std::string type() const override { return "Bass"; }
      std::string style() const override { return "Bass"; }
      std::string prefix() const override { return "BS"; }
      void build(RecipeContext& c) const override;
  };

  // Arp inside the preset: plucked steps, accents, ping-pong; sometimes a vocal timbre
  class ArpRecipe : public Recipe {
    public:
      std::string type() const override { return "Arp"; }
      std::string style() const override { return "Arp"; }
      std::string prefix() const override { return "ARP"; }
      void build(RecipeContext& c) const override;
  };

  // Sequence inside the preset: hold a note and a melodic line plays in the scale
  class SequenceRecipe : public Recipe {
    public:
      std::string type() const override { return "Sequence"; }
      std::string style() const override { return "Sequence"; }
      std::string prefix() const override { return "SQ"; }
      void build(RecipeContext& c) const override;
  };

  // Pluck: instant attack, short filter decay, delay
  class PluckRecipe : public Recipe {
    public:
      std::string type() const override { return "Pluck"; }
      std::string style() const override { return "Pluck"; }
      std::string prefix() const override { return "PL"; }
      void build(RecipeContext& c) const override;
  };

  // Stab: wide short chord hit, sometimes a unison stack of fifths or octaves
  class StabRecipe : public Recipe {
    public:
      std::string type() const override { return "Stab"; }
      std::string style() const override { return "Keys"; }
      std::string prefix() const override { return "ST"; }
      void build(RecipeContext& c) const override;
  };

  // Keys: FM bell or electric piano, velocity response, chorus
  class KeysRecipe : public Recipe {
    public:
      std::string type() const override { return "Keys"; }
      std::string style() const override { return "Keys"; }
      std::string prefix() const override { return "KY"; }
      void build(RecipeContext& c) const override;
  };

  // Pad / atmosphere: slow attack, three oscillators, deep reverb, drifting waves
  class PadRecipe : public Recipe {
    public:
      std::string type() const override { return "Pad"; }
      std::string style() const override { return "Pad"; }
      std::string prefix() const override { return "PD"; }
      void build(RecipeContext& c) const override;
  };

  // Voice: vocal wave and formant filter, vowels move with the tempo
  class VocalRecipe : public Recipe {
    public:
      std::string type() const override { return "Vocal"; }
      std::string style() const override { return "Vocal"; }
      std::string prefix() const override { return "VC"; }
      void build(RecipeContext& c) const override;
  };

  // Effects: risers, falls, psychedelic textures with noise and random motion
  class SfxRecipe : public Recipe {
    public:
      std::string type() const override { return "SFX"; }
      std::string style() const override { return "SFX"; }
      std::string prefix() const override { return "SFX"; }
      void build(RecipeContext& c) const override;
  };

  // Drums: kick and zap are a sine with falling pitch; hat and perc are filtered noise
  class PercussionRecipe : public Recipe {
    public:
      std::string type() const override { return "Percussion"; }
      std::string style() const override { return "Percussion"; }
      std::string prefix() const override { return "PERC"; }
      void build(RecipeContext& c) const override;
  };

} // namespace gen
} // namespace corpuscular
