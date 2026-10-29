# Модули JUCE 6.0.5 в той правке, что лежит в исходниках Vital (только эти 14 модулей).
juce_add_modules(
    INSTALL_PATH "include/JUCE-${JUCE_VERSION}/modules"
    ALIAS_NAMESPACE juce
    juce_audio_basics
    juce_audio_devices
    juce_audio_formats
    juce_audio_plugin_client
    juce_audio_processors
    juce_audio_utils
    juce_core
    juce_data_structures
    juce_dsp
    juce_events
    juce_graphics
    juce_gui_basics
    juce_gui_extra
    juce_opengl)
