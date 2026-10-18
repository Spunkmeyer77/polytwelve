cmake_minimum_required(VERSION 3.22)

project(Polytwelve VERSION 0.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# JUCE: use a local checkout if -DJUCE_DIR=... is given, otherwise download it
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(P12_FORMATS AU VST3 Standalone)
elseif(WIN32)
    set(P12_FORMATS VST3 Standalone)
else()
    set(P12_FORMATS VST3 LV2 Standalone)
endif()

# Copy the built plug-ins into the user's plug-in folders (turned off on the build servers)
option(P12_COPY_PLUGINS "Copy plug-ins to the plug-in folders after building" ON)

juce_add_plugin(Polytwelve
    COMPANY_NAME "Ticco Ross"
    PLUGIN_MANUFACTURER_CODE Tcro
    PLUGIN_CODE Ptwv
    FORMATS ${P12_FORMATS}
    LV2URI "urn:ticcoross:polytwelve"
    PRODUCT_NAME "Polytwelve"
    BUNDLE_ID com.ticcoross.polytwelve
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD ${P12_COPY_PLUGINS})

target_sources(Polytwelve PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PresetManager.cpp)

target_compile_definitions(Polytwelve PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Polytwelve
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optional: headless test app (renders every program to WAV and snapshots the editor)
option(P6_BUILD_TESTS "Build the plugin test app" OFF)
if(P6_BUILD_TESTS)
    juce_add_console_app(PolytwelveTest PRODUCT_NAME "PolytwelveTest")
    target_sources(PolytwelveTest PRIVATE tools/plugin_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/PresetManager.cpp)
    target_compile_definitions(PolytwelveTest PRIVATE
        JucePlugin_Name="Polytwelve" JucePlugin_VersionString="${PROJECT_VERSION}" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(PolytwelveTest PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
