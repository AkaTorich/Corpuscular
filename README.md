# Corpuscular

Corpuscular is a spectral warping wavetable synthesizer: a VST3 plugin and a standalone app for Windows, macOS
(plus Audio Unit) and Linux. It is free software under the GNU GPL v3 (see `LICENSE` and the License section below).

Author of the changes: AkaTorich, 2026.

## Changes to the original code

- **Name and identity.** Product name Corpuscular by AkaTorich, own plugin codes (`Ovkx` / `Crpc`),
  bundle id `com.ovk.corpuscular`, own logo and icons. Version 0.1.0.
- **Sound engine** unchanged: the same preset renders bit for bit the same audio as before the changes.
- **Tabs follow the path of the sound:** ARP → OSC → ROUTE → FILTER → AMP → FX → MOD → MATRIX → SETUP → GEN.
  - ARP — a built-in sequencer with the Melody Atoms generators (PSY ARP, ARP with 4th-bar endings, ARP STYLED,
    BASS, PAD; keys, scale library and the same settings as Melody Atoms). GENERATE works while it plays.
    The held note (DAW, MIDI keyboard, on-screen keys) is the tonic; LATCH keeps playing after release.
    In a playing DAW it follows the host tempo and bar grid; otherwise it starts from the pressed note.
    RATE 1/32…1/4 with triplets, OCTAVE, GATE, LENGTH. The piano roll edits everything: add, move, resize,
    delete, box select, copy/paste, duplicate, undo/redo, zoom and scroll in time and pitch.
    The pattern and the ARP settings are saved in the preset and in the DAW project (key `corpuscular_arp`,
    which Vital ignores); a preset without one keeps the current pattern. With ARP off the MIDI passes untouched.
    `--headless --render` plays the pattern too when the preset has ARP on.
    MODE, KEY, SCALE GROUP, SCALE and the mode list open Vital's popup list (click the middle; the sides step).
    SAVE stores the pattern with all ARP settings in `Documents\Corpuscular\Arps\*.corparp` (Vital's save dialog);
    SAVED ARP steps through them with its arrows, a click on the name opens Vital's file browser. Loading keeps ON as it is.
    DRAG MIDI: drag it into the DAW to drop the pattern as a MIDI clip (as it plays: RATE, GATE, OCTAVE, the clip
    as long as the pattern); a click saves the same `.mid` to `Documents\Corpuscular\Arps\MIDI` (a new file each time,
    named after the saved arp or the key and mode). `ArpSequencer::writeMidiFile` writes it.
    The other way: a MIDI clip dragged from the DAW (or a `.mid`/`.midi` file) dropped on the tab becomes the pattern —
    all tracks, on the grid of the current RATE, whole bars, in the current KEY; UNDO brings the old pattern back
    (`ArpSequencer::readMidiFile`).
  - OSC — the three oscillators and the sample source, full height.
  - ROUTE — where every source goes (filter 1, filter 2, both, effects, direct) and whether the filters run in series, as a diagram and a grid.
  - FILTER — filter 1 and filter 2, large.
  - AMP — the envelopes large, voices, velocity, glide.
  - FX — the effect chain, with a tenth effect VOCODER (QuantumVocoder) at the end of the list:
    the voice comes from the sidechain bus in a DAW (else the main input), in the standalone app from the audio input;
    CARRIER SYNTH lets the synth itself speak, TABLE uses a wavetable (built-in, or any `.vitaltable/.wav/.flac/.wt`
    from the wavetable folders) played AUTO (follows the voice's pitch), FIXED (NOTE + OCTAVE) or KEY (last played note).
    Knobs: POSITION, DETUNE, GLIDE, UNISON, BANDS 8–40, MIX, LEVEL, WIDTH, FORMANT ±12 st, ATTACK, RELEASE, AIR,
    NOISE (consonants), GATE, TILT, SPREAD (bands across the stereo field), FREEZE (holds the spectrum). The window under the table name shows the table's frame at POS, with the voice's band levels over it.
    With no voice at all (two seconds of digital silence on the input) the synth's own sound becomes the voice,
    so every knob still shapes the sound; NOISE and GATE only matter with a real voice.
    `--headless --render preset.vital --voice voice.wav` feeds a voice file (looped) to the vocoder.
    Presets keep Vital's 9-effect order and store the vocoder's slot and table under `corpuscular_vocoder`.
  - MOD — envelopes, LFOs and random sources across the whole window.
  - MATRIX — the modulation matrix.
  - SETUP — oscillator options, voice settings, oversampling, display, skin.
  - GEN — the built-in preset and wavetable generators (psytrance-oriented). Everything made there goes straight into Corpuscular:
    - NEW PRESET — a preset of the chosen type, saved to `User\Presets\Generated` and loaded;
    - NEW + WAVETABLES — the same plus a fresh wavetable in every oscillator the preset uses;
    - SIMILAR — a relative of the current preset: continuous knobs move a little, switches, tuning and wavetables stay,
      stepped LFO melodies swap a few steps (same scale); saved to `User\Presets\Generated\Similar` as "Name ~ Word";
    - BACK / NEXT — the last 20 presets made on the tab (kept in `User\generator_history.txt`);
    - MAKE BANK — any number of presets of any types as a bank folder in the data folder; the preset browser opens on it;
    - NEW TO OSC 1/2/3 — a wavetable (13 types, 8–256 frames, 256–4096 samples) loaded into that oscillator and saved to `User\Wavetables\Generated`;
    - EXPORT FILE — the wavetable for other synths: wavetable WAV (2048-sample frames), WAV 32/16, `.wt`, raw float;
    - VOCODER — OFF, ON (every new preset and bank preset gets the vocoder) or SOME (about one in four).
- **Generated library on the first launch.** When `Documents\Corpuscular` has no `Presets`, `Wavetables`, `LFOs` or
  `Skins` folder, the synth builds them in the background (512 presets in 11 types, 256 wavetables in 13 types, 256 LFO
  shapes in 16 families, 16 genre skins) with a progress bar in place of the keyboard. `Presets\Vocoder` gets 64 more
  presets with the vocoder on (style Vocal, SYNTH or TABLE carrier with a built-in table); a library made before it
  only gets this folder added. The skins (Psytrance, Techno,
  House, Trance, Drum and Bass, Dubstep, Hip-Hop, Ambient, Synthwave, Pop, Rock, Metal, Jazz, Classical, Reggae, Disco)
  are recoloured from Vital's original skin and appear in SETUP → Skin. Each folder is written as `<name>.generating` and renamed
  when complete; an existing folder is never rebuilt. `Corpuscular.exe --headless --generate` builds the missing parts right away.
- **Preset styles** Arp, Pluck and Vocal added to the browser filters and the save dialog (12 styles, 4 per row);
  generated presets use them.
- **Modulation sources stay visible** on every tab, so drag and drop onto any knob still works.
- **Default skin "Corpuscular TI"**: charcoal panels, red LED accents, wooden side panels.
  A new skin colour "Side Panel" draws the side panels; skins without it have no side panels.
  Menu: Load Skin..., Open Skin Designer, Back to Corpuscular TI Skin.
- **Signature skins** Psy Night Stage, Psy Day Stage and Neon Tubes (from the preset generator) are built in and
  copied to `Documents\Corpuscular\Skins` on launch when missing.
- **No network at all.** No sign in, no downloads, no update checks, no store, no text to wavetable.
  `CORPUSCULAR_NO_NETWORK` makes every web request in JUCE fail and refuses to open remote links.
- **Data folder** `Documents\Corpuscular`. The Vital data folder (if present) is scanned too,
  so existing Vital presets, wavetables and skins show up in the browsers.
- **File formats unchanged:** `.vital`, `.vitalbank`, `.vitalskin`, `.vitaltable`, `.vitallfo`;
  presets are written as Vital 1.0.7 and open in Vital.

## Where things are

| Path | What |
|---|---|
| `CMakeLists.txt`, `build.cmd` | Build (CMake; `build.cmd` for Visual Studio 2026) |
| `.github/workflows/build.yml` | Builds for Windows, macOS and Linux; a `v*` tag publishes them as a release |
| `src/common/corpuscular_info.h` | Name, version, format version |
| `src/interface/editor_sections/full_interface.*` | Window layout and tabs |
| `src/interface/editor_sections/header_section.cpp` | Tab names, logo |
| `src/interface/editor_sections/routing_section.*` | ROUTE tab |
| `src/interface/editor_sections/generator_section.*` | GEN tab |
| `src/generator/` | Generator engines: `preset_generator.h` (presets, scales, banks), `wavetable_generator.h` (wavetables and their file formats) |
| `src/generator/lfo_generator.*` | LFO shape generator (16 families) |
| `src/generator/preset_mutator.*` | SIMILAR: makes a relative of a preset |
| `src/generator/skin_generator.*` | 16 genre skins (palettes, contrast check, side panel colour) |
| `src/melody_atoms/` | Melody Atoms generators (from PresetsGenerators-main\MelodyAtoms), `melody_atoms/scales.json` |
| `src/common/arp_sequencer.*` | ARP sequencer: tonic from the held note, host sync, preset state |
| `src/interface/editor_sections/arp_section.*`, `arp_piano_roll.*` | ARP tab and its piano roll |
| `src/unity_build/generator.cpp` | Builds the generator (C++17) |
| `src/quantum_vocoder/` | QuantumVocoder DSP (`qv_*`, C++17): carrier, pitch tracker, wavetables, `qv_synth_vocoder.*` |
| `src/synthesis/modules/vocoder_module.*` | The vocoder in the effect chain; the sidechain input lives in `sound_engine.*` |
| `src/interface/editor_sections/vocoder_section.*` | VOCODER panel on the FX tab, `icons/vocoder.svg` |
| `branding/psy_*.vitalskin`, `branding/neon_tubes.vitalskin` | Signature skins built into the binary |
| `src/common/corpuscular_library.*` | First-launch library build (background thread, progress) |
| `src/interface/editor_sections/library_progress_section.*` | The progress bar shown during that build |
| `src/standalone/generate_command.*` | `--generate` command line |
| `src/interface/look_and_feel/skin.*` | Skins, the Side Panel colour |
| `default.vitalskin` | Corpuscular TI skin built into the binary |
| `branding/` | Logo sources, icons, preview, the original Vital skin |
| `icons/corpuscular_*.svg` | Logo as used inside the window |
| `third_party/JUCE` | JUCE 6.0.5 with the original project's module changes plus the network switch |
| `fonts/` | Built-in fonts and their licenses (`fonts/README.txt`) |

Every Vital file changed for Corpuscular carries the line `Modified for Corpuscular, 2026.` in its header.

## Building

Ready builds for all three systems are on the Releases page (made by `.github/workflows/build.yml`).

**Windows** — Visual Studio 2022 or newer with the C++ desktop workload:

```bat
build.cmd
```

`build.cmd` maps the project folder to drive `X:` (JUCE tools fail on non-ASCII paths),
configures CMake into `build\` and builds Release. Plain `cmake -S . -B build -A x64` and
`cmake --build build --config Release` work too when the path is ASCII.

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

Results are in `build/CorpuscularApp_artefacts/Release` (the app) and
`build/CorpuscularPlugin_artefacts/Release` (`VST3/Corpuscular.vst3`, on macOS also `AU/Corpuscular.component`).

Rendering a preset to WAV without a window (the working folder is where the WAV goes):

```bat
Corpuscular.exe --headless --render preset.vital -m 48 -l 4 -b 140
```

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
