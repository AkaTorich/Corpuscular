# Corpuscular

Corpuscular is a spectral warping wavetable synthesizer: a VST3 plugin and a standalone app for Windows, macOS
(plus Audio Unit) and Linux. Version 0.1.10. It is free software under the GNU GPL v3 (see `LICENSE` and the License
section below).

Author of the changes: AkaTorich, 2026.

## What it is

- **Tabs follow the path of the sound:** ARP → OSC → ROUTE → FILTER → AMP → FX → MOD → MATRIX → SETUP → GEN.
  - ARP — a built-in sequencer with the Melody Atoms generators and a piano roll. The held note is the tonic; it
    follows the DAW tempo and bar grid. HARMONIC lays the melody over a chord progression, VELOCITY gives the notes
    a musical strength. Patterns are saved in the preset and in `Documents\Corpuscular\Arps`; DRAG MIDI drops the
    pattern into the DAW as a MIDI clip, and a MIDI clip dropped on the tab becomes the pattern.
  - OSC — the three oscillators and the sample source.
  - ROUTE — where every source goes (filter 1, filter 2, both, effects, direct).
  - FILTER — filter 1 and filter 2.
  - AMP — the envelopes, voices, velocity, glide.
  - FX — the effect chain with a tenth effect, VOCODER: a wavetable sounds and the synth gives it its formants.
  - MOD — envelopes, LFOs and random sources.
  - MATRIX — the modulation matrix.
  - SETUP — oscillator options, voice settings, oversampling, display, skin.
  - GEN — preset, bank, wavetable and skin generators; everything made there loads into the synth at once.
- **Library on the first launch:** presets, wavetables, LFO shapes and skins are generated in the background into
  `Documents\Corpuscular`; an existing folder is never rebuilt.
- **Preview on preset switch:** a picked preset plays one note for 5 seconds; while the DAW plays there is no
  preview, and a note the DAW holds goes on with the new preset (the standalone app has no transport and always
  previews).
- **Undo / redo** for every control: the two arrows right of the preset name.
- **Default skin "Corpuscular TI"**, plus the built-in skins Psy Night Stage, Psy Day Stage and Neon Tubes.
- **No network at all:** no sign in, downloads or update checks (`CORPUSCULAR_NO_NETWORK`).
- **Same note repeated:** it goes back to the voice still playing it, so a looped note has no click at the seam.
- **File formats** as in Vital: `.vital`, `.vitalbank`, `.vitalskin`, `.vitaltable`, `.vitallfo`; presets are
  written as Vital 1.0.7 and open in Vital.

## Data folder

Only `Documents\Corpuscular` (macOS `~/Music/Corpuscular`, Linux `~/.local/share/corpuscular`): presets, wavetables,
LFOs, samples, skins, banks and arps. Vital's folder and settings are never read. Settings, favorites and the skin
choice are `Corpuscular.config`, `Corpuscular.favorites` and `Corpuscular.skin` in `AppData\Roaming\corpuscular`.
A missing folder is made with `User\Presets`, `User\Wavetables`, `User\LFOs`, `User\Samples`, `User\Skins`.

## Installing

Ready builds for all three systems are on the Releases page.

- **Windows:** copy `Corpuscular.vst3` to `C:\Program Files\Common Files\VST3`; `Corpuscular.exe` runs on its own.
- **macOS:** copy `Corpuscular.vst3` to `~/Library/Audio/Plug-Ins/VST3`, `Corpuscular.component` to
  `~/Library/Audio/Plug-Ins/Components`, `Corpuscular.app` to Applications. The builds are not signed: allow them once
  with `xattr -dr com.apple.quarantine Corpuscular.app Corpuscular.vst3 Corpuscular.component`.
- **Linux:** copy `Corpuscular.vst3` to `~/.vst3`; `Corpuscular` runs on its own.

## Building

**Windows** — Visual Studio 2022 or newer with the C++ desktop workload:

```bat
build.cmd
```

`build.cmd` maps the project folder to drive `X:` (JUCE tools fail on non-ASCII paths), configures CMake into
`build\` and builds Release.

**macOS** — Xcode command line tools, CMake and Ninja:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build
```

**Linux** (Debian/Ubuntu packages):

```sh
sudo apt install build-essential cmake ninja-build libasound2-dev libjack-jackd2-dev libfreetype6-dev \
  libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxrender-dev libgl1-mesa-dev libglu1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Results: `build/CorpuscularApp_artefacts/Release` (the app) and `build/CorpuscularPlugin_artefacts/Release`
(`VST3/Corpuscular.vst3`, on macOS also `AU/Corpuscular.component`). `.github/workflows/build.yml` builds all three
systems; a `v*` tag publishes them as a release.

## Checks

```bat
rem Render a preset to WAV (into the working folder)
Corpuscular.exe --headless --render preset.vital -m 48 -l 4 -b 140

rem One note repeated like a looped DAW clip, 1 and 8 voices: numbers near 1.00 mean no click at the seam;
rem then a preset switched under a held note: near 1.00 while the DAW plays (the note goes on)
Corpuscular.exe --headless --retrigger-test <folder>

rem The same through the installed plugin, the way a DAW plays it (tools/loop_render, built with its build.cmd)
LoopRender.exe <folder> [plugin.vst3]

rem Generated presets of every type, each heard before it is kept
Corpuscular.exe --headless --audition-test <folder> --count N

rem Build the missing parts of the library right away
Corpuscular.exe --headless --generate
```

## Where things are

| Path | What |
|---|---|
| `CMakeLists.txt`, `build.cmd` | Build |
| `.github/workflows/build.yml` | Builds for Windows, macOS and Linux, releases |
| `src/common/corpuscular_info.h` | Name, version, format version |
| `src/common/load_save.*` | Data folder, settings files, preset and file loading |
| `src/common/corpuscular_library.*` | First-launch library |
| `src/common/arp_sequencer.*`, `arp_dynamics.*` | ARP sequencer and note strength |
| `src/interface/editor_sections/preset_preview.*` | Preview note on preset switch, silent while the DAW plays |
| `src/common/preset_audition.*` | Plays a generated preset, rejects silent ones, sets the level |
| `src/melody_atoms/` | Melody Atoms generators and chord progressions |
| `src/generator/` | Preset, wavetable, LFO and skin generators |
| `src/quantum_vocoder/`, `src/synthesis/modules/vocoder_module.*` | Vocoder |
| `src/synthesis/` | Sound engine |
| `src/interface/editor_sections/` | Window, tabs and panels |
| `src/interface/look_and_feel/skin.*`, `default.vitalskin`, `branding/` | Skins, logo, icons |
| `src/standalone/` | Standalone app and its command line (`--generate`, `--retrigger-test`) |
| `tools/loop_render/` | LoopRender: loop check through the installed plugin |
| `third_party/` | JUCE 6.0.5, VST3 SDK, json, concurrentqueue, kissfft |
| `fonts/` | Built-in fonts and their licenses |

## License

Corpuscular is free software: you can redistribute it and/or modify it under the terms of the
GNU General Public License version 3 or (at your option) any later version (`LICENSE`).
It comes with NO WARRANTY. Anyone who receives a build is entitled to this source.

Corpuscular is derived from the GPLv3 source code of the Vital synthesizer, Copyright 2013-2020 Matt Tytel.
It is an independent project, not affiliated with or endorsed by Vital Audio or Matt Tytel; the name Vital
appears here only to state where the code and the file formats come from. The original copyright notices
are kept in every source file, and every original file changed for Corpuscular carries the line
`Modified for Corpuscular, 2026.` Changes: Copyright 2026 AkaTorich, same license.

Rules of the original project that this fork follows:

- The names "Vital", "Vital Audio", "Tytel" and "Matt Tytel" are not used for marketing or to name builds.
- No build connects to any web service of vital.audio (the network code is removed or switched off).
- The presets that come with the free version of Vital are not included or distributed.
- Builds are not distributed through the iOS App Store (not compatible with GPLv3).

Third-party components:

| Component | License |
|---|---|
| JUCE 6.0.5 (`third_party/JUCE`) | GPLv3 (JUCE's open source option, `third_party/JUCE/LICENSE.md`) |
| VST3 SDK (`third_party/VST_SDK`, and the copy inside JUCE) | GPLv3 option of the Steinberg dual license (`LICENSE.txt`) |
| nlohmann/json (`third_party/json`) | MIT |
| moodycamel ConcurrentQueue (`third_party/concurrentqueue`) | Simplified BSD / Boost (`LICENSE.md`) |
| KISS FFT (`third_party/kissfft`) | BSD-3-Clause (`COPYING`) |
| Lato, Montserrat fonts | SIL Open Font License 1.1 (`fonts/OFL-*.txt`) |
| Roboto, Open Sans, Droid Sans Mono fonts | Apache License 2.0 (`fonts/LICENSE`) |

VST is a registered trademark of Steinberg Media Technologies GmbH. Audio Units is a trademark of Apple Inc.
