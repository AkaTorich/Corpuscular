# Corpuscular — синтезатор на исходниках Vital (Matt Tytel, GPLv3).
# Сборка: build.cmd (Windows, Visual Studio 2026, с подставного диска — JUCE не любит кириллицу в пути);
# macOS и Linux — обычный cmake, см. README и .github/workflows/build.yml.
cmake_minimum_required(VERSION 3.15)

project(Corpuscular VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

add_subdirectory(third_party/JUCE)

set(CORPUSCULAR_COMPANY "AkaTorich")
set(CORPUSCULAR_NAME "Corpuscular")

# Вшиваемые файлы: значки, шрифты, строи, скин по умолчанию
file(GLOB CORPUSCULAR_ICONS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*.svg")
file(GLOB CORPUSCULAR_FONTS "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttf" "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.otf")
file(GLOB CORPUSCULAR_TUNINGS "${CMAKE_CURRENT_SOURCE_DIR}/tunings/*.scl")
juce_add_binary_data(CorpuscularData
    SOURCES ${CORPUSCULAR_ICONS} ${CORPUSCULAR_FONTS} ${CORPUSCULAR_TUNINGS}
            "${CMAKE_CURRENT_SOURCE_DIR}/default.vitalskin"
            "${CMAKE_CURRENT_SOURCE_DIR}/branding/vital_original.vitalskin"
            "${CMAKE_CURRENT_SOURCE_DIR}/branding/psy_night_stage.vitalskin"
            "${CMAKE_CURRENT_SOURCE_DIR}/branding/psy_day_stage.vitalskin"
            "${CMAKE_CURRENT_SOURCE_DIR}/branding/neon_tubes.vitalskin"
            "${CMAKE_CURRENT_SOURCE_DIR}/melody_atoms/scales.json")
# The plugin is a shared library: on Linux its static parts must be position independent
set_target_properties(CorpuscularData PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

set(CORPUSCULAR_INCLUDES
    src/common src/common/wavetable
    src/interface/editor_components src/interface/editor_sections src/interface/look_and_feel
    src/interface/wavetable src/interface/wavetable/editors src/interface/wavetable/overlays
    src/synthesis/synth_engine src/synthesis/effects src/synthesis/filters src/synthesis/framework
    src/synthesis/lookups src/synthesis/modulators src/synthesis/modules src/synthesis/producers
    src/synthesis/utilities src/generator src/melody_atoms src/quantum_vocoder third_party)

set(CORPUSCULAR_SHARED_SOURCES
    src/unity_build/common.cpp
    src/unity_build/interface_editor_components.cpp
    src/unity_build/interface_editor_sections.cpp
    src/unity_build/interface_editor_sections2.cpp
    src/unity_build/interface_look_and_feel.cpp
    src/unity_build/interface_wavetable.cpp
    src/unity_build/synthesis.cpp
    src/unity_build/generator.cpp)

# Melody Atoms generators for the ARP tab, each file on its own (they share helper names)
set(CORPUSCULAR_MELODY_ATOMS_SOURCES
    src/melody_atoms/MusicData.cpp
    src/melody_atoms/ArpGen.cpp
    src/melody_atoms/ArpsyGen.cpp
    src/melody_atoms/ArpTurnaround.cpp
    src/melody_atoms/BassGen.cpp
    src/melody_atoms/Generators.cpp)
list(APPEND CORPUSCULAR_SHARED_SOURCES ${CORPUSCULAR_MELODY_ATOMS_SOURCES})

# QuantumVocoder DSP for the VOCODER effect
set(CORPUSCULAR_VOCODER_SOURCES
    src/quantum_vocoder/qv_carrier_synth.cpp
    src/quantum_vocoder/qv_pitch_tracker.cpp
    src/quantum_vocoder/qv_wavetable.cpp
    src/quantum_vocoder/qv_wavetable_formats.cpp
    src/quantum_vocoder/qv_built_in_tables.cpp
    src/quantum_vocoder/qv_synth_vocoder.cpp)
list(APPEND CORPUSCULAR_SHARED_SOURCES ${CORPUSCULAR_VOCODER_SOURCES})

# The preset generator, Melody Atoms and the vocoder DSP are C++17; the Vital code keeps its own standard
set_source_files_properties(src/unity_build/generator.cpp ${CORPUSCULAR_MELODY_ATOMS_SOURCES}
                            ${CORPUSCULAR_VOCODER_SOURCES}
                            PROPERTIES COMPILE_OPTIONS "$<IF:$<CXX_COMPILER_ID:MSVC>,/std:c++17,-std=c++17>")

# JUCE_DISPLAY_SPLASH_SCREEN=0 is allowed because JUCE is used under its GPLv3 license
set(CORPUSCULAR_DEFINES
    CORPUSCULAR_NO_NETWORK=1
    NO_AUTH=1
    JUCE_OPENGL3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)
if(MSVC)
    # MSVC doesn't announce SSE2 on x64; Vital's SIMD code looks for __SSE2__ (gcc and clang define it themselves)
    list(APPEND CORPUSCULAR_DEFINES __SSE2__=1 WINVER=0x0601 _WIN32_WINNT=0x0601 _ENABLE_EXTENDED_ALIGNED_STORAGE)
endif()

# Plugin formats: VST3 everywhere, Audio Unit on macOS too
set(CORPUSCULAR_FORMATS VST3)
if(APPLE)
    list(APPEND CORPUSCULAR_FORMATS AU)
endif()

set(CORPUSCULAR_MODULES
    juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats
    juce::juce_audio_processors juce::juce_audio_utils juce::juce_core juce::juce_data_structures
    juce::juce_dsp juce::juce_events juce::juce_graphics juce::juce_gui_basics juce::juce_gui_extra
    juce::juce_opengl)

function(corpuscular_common target)
    target_include_directories(${target} PRIVATE ${CORPUSCULAR_INCLUDES})
    target_compile_definitions(${target} PRIVATE ${CORPUSCULAR_DEFINES})
    target_link_libraries(${target} PRIVATE CorpuscularData ${CORPUSCULAR_MODULES}
        juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
    if(MSVC)
        target_compile_options(${target} PRIVATE /bigobj /fp:fast /MP /wd4100 /wd4244 /wd4267 /wd4305 /wd4456 /wd4458)
    endif()
endfunction()

# Плагин VST3
juce_add_plugin(CorpuscularPlugin
    PRODUCT_NAME "${CORPUSCULAR_NAME}"
    COMPANY_NAME "${CORPUSCULAR_COMPANY}"
    BUNDLE_ID "com.ovk.corpuscular"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/branding/icon_512.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/branding/icon_256.png"
    PLUGIN_MANUFACTURER_CODE Ovkx
    PLUGIN_CODE Crpc
    FORMATS ${CORPUSCULAR_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(CorpuscularPlugin)
target_sources(CorpuscularPlugin PRIVATE ${CORPUSCULAR_SHARED_SOURCES} src/unity_build/plugin.cpp)
target_include_directories(CorpuscularPlugin PRIVATE src/plugin)
target_link_libraries(CorpuscularPlugin PRIVATE juce::juce_audio_plugin_client)
target_compile_definitions(CorpuscularPlugin PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)
corpuscular_common(CorpuscularPlugin)

# Отдельная программа (со своей командной строкой: --headless --render и др.)
juce_add_gui_app(CorpuscularApp
    PRODUCT_NAME "${CORPUSCULAR_NAME}"
    COMPANY_NAME "${CORPUSCULAR_COMPANY}"
    BUNDLE_ID "com.ovk.corpuscular.app"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/branding/icon_512.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/branding/icon_256.png")
juce_generate_juce_header(CorpuscularApp)
target_sources(CorpuscularApp PRIVATE ${CORPUSCULAR_SHARED_SOURCES} src/unity_build/standalone.cpp)
target_include_directories(CorpuscularApp PRIVATE src/standalone)
if(WIN32)
    target_compile_definitions(CorpuscularApp PRIVATE JUCE_WASAPI=1 JUCE_DIRECTSOUND=1)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(CorpuscularApp PRIVATE JUCE_ALSA=1 JUCE_JACK=1)
endif()
corpuscular_common(CorpuscularApp)
