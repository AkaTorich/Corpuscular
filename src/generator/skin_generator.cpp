/* Corpuscular skin generator
 *
 * Part of Corpuscular (a fork of vital), GPLv3.
 */

#include "skin_generator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace corpuscular {
namespace gen {

  namespace {
    struct SkinGenColour {
      double a = 1.0, r = 0.0, g = 0.0, b = 0.0;

      static SkinGenColour parse(const std::string& text) {
        SkinGenColour colour;
        uint32_t value = (uint32_t)std::stoul(text, nullptr, 16);
        colour.a = ((value >> 24) & 0xff) / 255.0;
        colour.r = ((value >> 16) & 0xff) / 255.0;
        colour.g = ((value >> 8) & 0xff) / 255.0;
        colour.b = (value & 0xff) / 255.0;
        return colour;
      }

      std::string text() const {
        auto channel = [](double v) { return (unsigned)std::lround(std::min(1.0, std::max(0.0, v)) * 255.0); };
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x", channel(a), channel(r), channel(g), channel(b));
        return buffer;
      }

      void toHsl(double& h, double& s, double& l) const {
        double high = std::max(r, std::max(g, b));
        double low = std::min(r, std::min(g, b));
        l = (high + low) / 2.0;
        if (high == low) {
          h = 0.0;
          s = 0.0;
          return;
        }
        double d = high - low;
        s = l > 0.5 ? d / (2.0 - high - low) : d / (high + low);
        if (high == r)
          h = (g - b) / d + (g < b ? 6.0 : 0.0);
        else if (high == g)
          h = (b - r) / d + 2.0;
        else
          h = (r - g) / d + 4.0;
        h *= 60.0;
      }

      static SkinGenColour fromHsl(double alpha, double h, double s, double l) {
        h = std::fmod(std::fmod(h, 360.0) + 360.0, 360.0) / 360.0;
        s = std::min(1.0, std::max(0.0, s));
        l = std::min(1.0, std::max(0.0, l));
        auto hue = [](double p, double q, double t) {
          if (t < 0.0) t += 1.0;
          if (t > 1.0) t -= 1.0;
          if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
          if (t < 0.5) return q;
          if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
          return p;
        };
        SkinGenColour colour;
        colour.a = alpha;
        if (s == 0.0) {
          colour.r = colour.g = colour.b = l;
          return colour;
        }
        double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
        double p = 2.0 * l - q;
        colour.r = hue(p, q, h + 1.0 / 3.0);
        colour.g = hue(p, q, h);
        colour.b = hue(p, q, h - 1.0 / 3.0);
        return colour;
      }

      double luminance() const {
        auto linear = [](double c) { return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); };
        return 0.2126 * linear(r) + 0.7152 * linear(g) + 0.0722 * linear(b);
      }

      static double contrast(const SkinGenColour& x, const SkinGenColour& y) {
        double a = x.luminance() + 0.05;
        double b = y.luminance() + 0.05;
        return std::max(a, b) / std::min(a, b);
      }
    };

    // One genre: the grey scale (tint, lift, spread, light or dark), the hues of vital's five colour roles
    // (main, modulation, warm, hot, cold), how strong the colours are, corners and the side panels
    struct SkinGenTheme {
      const char* name;
      bool light;
      double tint_hue, tint_saturation, lift, spread;
      double saturation, lightness_shift;
      double roles[5];
      int shape;                 // 0 default, 1 rounded, 2 sharp
      const char* side_panel;    // ARGB
    };

    const std::vector<SkinGenTheme>& skinGenThemes() {
      static const std::vector<SkinGenTheme> themes = {
        { "Psytrance",     false, 280, 0.30, -0.06, 1.10, 1.50,  0.02, { 300, 150,  60, 330, 190 }, 1, "ff2a1240" },
        { "Techno",        false, 220, 0.04, -0.09, 1.00, 0.90,  0.00, { 195,   0,  40, 355, 210 }, 2, "00000000" },
        { "House",         false,  30, 0.15, -0.02, 1.00, 1.10,  0.02, {  35, 175,  50,  15, 200 }, 1, "ff5a3a22" },
        { "Trance",        false, 225, 0.30, -0.04, 1.05, 1.30,  0.04, { 200, 280,  50, 320, 230 }, 1, "ff10224a" },
        { "Drum and Bass", false, 160, 0.18, -0.07, 1.05, 1.30,  0.02, {  95, 180,  55,  20, 200 }, 2, "00000000" },
        { "Dubstep",       false, 270, 0.15, -0.08, 1.10, 1.50,  0.04, { 120, 285,  60, 330, 200 }, 2, "ff1b1b1b" },
        { "Hip-Hop",       false,  40, 0.12, -0.04, 1.00, 1.10,  0.00, {  45,  10,  30, 350, 210 }, 0, "ff3a2a14" },
        { "Ambient",       true,  200, 0.12,  0.00, 1.00, 0.55,  0.12, { 190, 150,  40, 330, 220 }, 1, "ffb9c6cc" },
        { "Synthwave",     false, 285, 0.35, -0.05, 1.05, 1.40,  0.04, { 320, 185,  40, 340, 200 }, 0, "ff2b0f3d" },
        { "Pop",           true,  330, 0.15,  0.00, 1.00, 1.10, -0.08, { 330, 190,  45, 350, 260 }, 1, "fff0c8dc" },
        { "Rock",          false,  10, 0.06, -0.05, 1.00, 1.10,  0.00, {  15, 200,  40,   0, 210 }, 2, "ff2a2522" },
        { "Metal",         false,   0, 0.02, -0.10, 1.10, 1.10,  0.00, {   0,   0,  30, 355,   0 }, 2, "ff5b6066" },
        { "Jazz",          false,  28, 0.20, -0.03, 1.00, 0.80,  0.00, {  40, 200,  30, 355, 210 }, 1, "ff4a2416" },
        { "Classical",     true,   45, 0.18,  0.00, 1.00, 0.70, -0.10, {  35, 210,  30, 350, 220 }, 0, "ff6b4226" },
        { "Reggae",        false, 120, 0.12, -0.04, 1.00, 1.20,  0.02, {  50, 130,  45, 355, 140 }, 1, "ff1f3a1a" },
        { "Disco",         false, 300, 0.20, -0.05, 1.05, 1.30,  0.04, {  48, 320,  30, 345, 190 }, 1, "ff3b1d3a" },
      };
      return themes;
    }

    int skinGenRole(double h) {
      h = std::fmod(std::fmod(h, 360.0) + 360.0, 360.0);
      if (h >= 245.0 && h < 290.0) return 0;
      if (h >= 120.0 && h < 190.0) return 1;
      if (h >= 20.0 && h < 70.0) return 2;
      if (h >= 290.0 || h < 20.0) return 3;
      return 4;
    }

    double skinGenRoleCenter(int role) {
      static const double kCenters[] = { 262.0, 165.0, 40.0, 350.0, 205.0 };
      return kCenters[role];
    }

    bool skinGenKeepsColour(const std::string& key) {
      return key == "Shadow" || key == "Overlay Screen";
    }

    std::string skinGenRecolor(const std::string& key, const std::string& text, const SkinGenTheme& theme) {
      if (skinGenKeepsColour(key))
        return text;

      SkinGenColour colour = SkinGenColour::parse(text);
      double h, s, l;
      colour.toHsl(h, s, l);
      if (s < 0.2 || colour.a == 0.0) {
        // Greys: a new lightness scale with a slight tint; a light theme turns the scale over
        double new_lightness = 0.5 + (l - 0.5) * theme.spread + theme.lift;
        if (theme.light)
          new_lightness = 1.0 - new_lightness * 0.92;
        double tint = theme.tint_saturation * (1.0 - std::abs(new_lightness - 0.5) * 1.2);
        return SkinGenColour::fromHsl(colour.a, theme.tint_hue, std::max(0.0, tint), new_lightness).text();
      }

      int role = skinGenRole(h);
      double new_hue = theme.roles[role] + (h - skinGenRoleCenter(role)) * 0.5;
      double new_saturation = std::min(1.0, s * theme.saturation);
      double new_lightness = std::max(0.25, std::min(0.85, l + theme.lightness_shift));
      return SkinGenColour::fromHsl(colour.a, new_hue, new_saturation, new_lightness).text();
    }

    // Makes `foreground` readable on `background` by moving its lightness
    void skinGenEnsureContrast(nlohmann::json& skin, const char* foreground, const char* background, double ratio) {
      if (!skin.count(foreground) || !skin.count(background))
        return;
      SkinGenColour back = SkinGenColour::parse(skin[background].get<std::string>());
      SkinGenColour front = SkinGenColour::parse(skin[foreground].get<std::string>());
      if (SkinGenColour::contrast(front, back) >= ratio)
        return;

      SkinGenColour best = front;
      for (bool up : { back.luminance() < 0.25, back.luminance() >= 0.25 }) {
        double h, s, l;
        front.toHsl(h, s, l);
        SkinGenColour trial = front;
        for (int i = 0; i < 40 && SkinGenColour::contrast(trial, back) < ratio; ++i) {
          l = up ? std::min(1.0, l + 0.03) : std::max(0.0, l - 0.03);
          trial = SkinGenColour::fromHsl(front.a, h, s, l);
        }
        if (SkinGenColour::contrast(trial, back) > SkinGenColour::contrast(best, back))
          best = trial;
        if (SkinGenColour::contrast(best, back) >= ratio)
          break;
      }
      skin[foreground] = best.text();
    }

    void skinGenShape(nlohmann::json& skin, int shape) {
      if (shape == 1) {
        skin["Body Rounding"] = 10.0;
        skin["Widget Rounded Corner"] = 14.0;
        skin["Label Rounding"] = 11.0;
        skin["Widget Line Width"] = 2.4;
        skin["Knob Arc Thickness"] = 3.4;
      }
      else if (shape == 2) {
        skin["Body Rounding"] = 0.0;
        skin["Widget Rounded Corner"] = 1.0;
        skin["Label Rounding"] = 2.0;
        skin["Widget Line Width"] = 1.6;
        skin["Knob Arc Thickness"] = 2.0;
      }
      if (shape != 0) {
        skin["Widget Fill Fade"] = 0.3;
        skin["Widget Fill Boost"] = 1.6;
      }
    }
  }

  std::vector<std::string> skinGenres() {
    std::vector<std::string> names;
    for (const SkinGenTheme& theme : skinGenThemes())
      names.push_back(theme.name);
    return names;
  }

  nlohmann::json makeGenreSkin(const nlohmann::json& base, const std::string& genre) {
    const std::vector<SkinGenTheme>& themes = skinGenThemes();
    const SkinGenTheme* theme = &themes.front();
    for (const SkinGenTheme& candidate : themes) {
      if (genre == candidate.name)
        theme = &candidate;
    }

    nlohmann::json skin = base;
    for (auto it = skin.begin(); it != skin.end(); ++it) {
      if (it.value().is_string())
        it.value() = skinGenRecolor(it.key(), it.value().get<std::string>(), *theme);
    }

    if (skin.count("overrides") && skin["overrides"].is_object()) {
      for (auto section = skin["overrides"].begin(); section != skin["overrides"].end(); ++section) {
        if (!section.value().is_object())
          continue;
        for (auto it = section.value().begin(); it != section.value().end(); ++it) {
          if (it.value().is_string())
            it.value() = skinGenRecolor(it.key(), it.value().get<std::string>(), *theme);
        }
      }
    }

    skinGenShape(skin, theme->shape);
    skinGenEnsureContrast(skin, "Body Text", "Body", 4.5);
    skinGenEnsureContrast(skin, "Heading Text", "Body Heading Background", 4.5);
    skinGenEnsureContrast(skin, "Preset Text", "Popup Selector Background", 4.5);
    skinGenEnsureContrast(skin, "Text Component Text", "Text Component Background", 4.5);
    skinGenEnsureContrast(skin, "UI Button Text", "UI Button", 3.5);
    skinGenEnsureContrast(skin, "Rotary Arc", "Body", 2.2);
    skinGenEnsureContrast(skin, "Widget Primary 1", "Widget Background", 2.5);
    skinGenEnsureContrast(skin, "Power Button On", "Body", 2.2);
    skinGenEnsureContrast(skin, "UI Action Button", "Body", 1.8);

    skin["Side Panel"] = theme->side_panel;
    return skin;
  }

} // namespace gen
} // namespace corpuscular
