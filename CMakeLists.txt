cmake_minimum_required(VERSION 3.22)

project(WinampClassicEqualizer VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

option(WCE_BUILD_PLUGIN "Build the VST3 plugin" ON)
option(WCE_BUILD_TESTS "Build the core unit tests" ON)

include(FetchContent)

add_library(classic_eq_core STATIC
    Source/Core/ClassicEqWindow.cpp
    Source/Core/ConstantQEq.cpp
    Source/Core/EqGain.cpp
    Source/Core/EqPresets.cpp
    Source/Core/EqSkin.cpp
    Source/Core/EqualizerDsp.cpp
    Source/Core/FourFrontEq.cpp
    Source/Core/Spline.cpp)
target_include_directories(classic_eq_core PUBLIC Source/Core)
set_target_properties(classic_eq_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(WCE_BUILD_PLUGIN)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)

    juce_add_plugin(WinampClassicEqualizer
        COMPANY_NAME "Alan Marcero"
        PRODUCT_NAME "Winamp Classic Equalizer"
        PLUGIN_MANUFACTURER_CODE Amrc
        PLUGIN_CODE Wceq
        FORMATS VST3
        VST3_CATEGORIES Fx EQ
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(WinampClassicEqualizer PRIVATE
        Source/Plugin/Parameters.cpp
        Source/Plugin/PluginEditor.cpp
        Source/Plugin/PluginProcessor.cpp)

    target_compile_definitions(WinampClassicEqualizer PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(WinampClassicEqualizer
        PRIVATE
            classic_eq_core
            juce::juce_audio_processors
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

if(WCE_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.9.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)

    add_executable(core_tests
        Tests/ConstantQEqTests.cpp
        Tests/EqGainTests.cpp
        Tests/EqualizerDspTests.cpp
        Tests/FourFrontEqTests.cpp
        Tests/PresetsAndSkinTests.cpp
        Tests/SplineAndUiTests.cpp)
    target_link_libraries(core_tests PRIVATE classic_eq_core Catch2::Catch2WithMain)

    enable_testing()
    include(Catch)
    catch_discover_tests(core_tests)
endif()
