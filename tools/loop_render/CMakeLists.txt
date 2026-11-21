# LoopRender — plays one note in a loop through the installed Corpuscular.vst3 offline and measures each repeat.
# Build: build.cmd (Code\Libraries\JUCE mapped to V:, this folder to Q:: JUCE tools dislike Cyrillic paths).
cmake_minimum_required(VERSION 3.22)
project(LoopRender VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OVK_LIBRARIES "V:/" CACHE PATH "Code\\Libraries: JUCE")
add_subdirectory(${OVK_LIBRARIES}/JUCE ${CMAKE_BINARY_DIR}/JUCE)

juce_add_console_app(LoopRender PRODUCT_NAME "LoopRender")
juce_generate_juce_header(LoopRender)
target_sources(LoopRender PRIVATE main.cpp)
target_compile_definitions(LoopRender PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)
target_link_libraries(LoopRender PRIVATE
    juce::juce_audio_processors juce::juce_audio_formats juce::juce_gui_basics
    juce::juce_recommended_config_flags)
if(MSVC)
    target_compile_options(LoopRender PRIVATE /MP /wd4100)
endif()
