cmake_minimum_required(VERSION 3.22)
project(MKWobbleBass VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MK_BUILD_PLUGIN "Build the JUCE plug-in targets (VST3 / AUv3 / Standalone)" ON)
option(MK_BUILD_TESTS "Build the portable DSP test suite and benchmark" ON)
option(MK_INSTALL_PLUGINS "Copy built plug-ins into the system plug-in folder" OFF)
set(MK_JUCE_SOURCE_DIR "" CACHE PATH "Path to a JUCE 9 checkout")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# iPadOS floor. Kept separate from CMAKE_OSX_DEPLOYMENT_TARGET because that
# variable also feeds the macOS-only juceaide host tool.
set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "17.0" CACHE STRING "iPadOS deployment target")

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(MK_TARGET_IOS TRUE)
else()
    set(MK_TARGET_IOS FALSE)
endif()

# ---------------------------------------------------------------------------
# Portable DSP core. No JUCE, no allocation on the audio thread, fixed-size
# state. Everything the tests and the benchmark exercise lives here.
# ---------------------------------------------------------------------------
add_library(mkwobble_core STATIC
    Source/DSP/Params.cpp
    Source/DSP/ParamStore.cpp
    Source/DSP/Wavetables.cpp
    Source/DSP/Effects.cpp
    Source/DSP/Engine.cpp
    Source/DSP/Presets.cpp
    Source/DSP/GatePresets.cpp
)
target_include_directories(mkwobble_core PUBLIC Source)
target_compile_definitions(mkwobble_core PUBLIC MK_CORE=1)

if (MSVC)
    target_compile_options(mkwobble_core PRIVATE /W3 /permissive- /fp:precise)
else()
    target_compile_options(mkwobble_core PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# The A10X is a 3-core Out-of-Order ARMv8 with a small L1/L2. -O3 plus
# -fno-math-errno lets exp2f/tanf in the coefficient paths become plain
# instructions; -ffast-math is deliberately avoided because the DSP relies on
# explicit denormal handling and NaN-free comparisons.
if (APPLE AND NOT MSVC)
    target_compile_options(mkwobble_core PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-fno-math-errno>
        $<$<CONFIG:Release>:-fno-trapping-math>
        $<$<CONFIG:Release>:-fomit-frame-pointer>)
endif()

if (MK_BUILD_TESTS AND NOT MK_TARGET_IOS)
    enable_testing()
    add_executable(mk_wobble_tests Tests/CoreTests.cpp)
    target_link_libraries(mk_wobble_tests PRIVATE mkwobble_core)
    add_test(NAME mk_wobble_tests COMMAND mk_wobble_tests)

    add_executable(mk_wobble_bench Tools/Benchmark.cpp)
    target_link_libraries(mk_wobble_bench PRIVATE mkwobble_core)
endif()

# ---------------------------------------------------------------------------
# JUCE wrapper. One shared engine, three plug-in formats.
# ---------------------------------------------------------------------------
if (MK_BUILD_PLUGIN)
    if (NOT MK_JUCE_SOURCE_DIR)
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
            set(MK_JUCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
        elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/JUCE/CMakeLists.txt")
            set(MK_JUCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/JUCE")
        endif()
    endif()

    if (MK_JUCE_SOURCE_DIR)
        add_subdirectory("${MK_JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/juce")
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 9.0.5
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    if (MK_TARGET_IOS)
        if (NOT CMAKE_GENERATOR STREQUAL "Xcode")
            message(FATAL_ERROR "Use the Xcode generator for the iOS AUv3 container.")
        endif()
        set(MK_FORMATS AUv3 Standalone)
    elseif (APPLE)
        set(MK_FORMATS VST3 AU Standalone)
        if (CMAKE_GENERATOR STREQUAL "Xcode")
            list(APPEND MK_FORMATS AUv3)
        endif()
    else()
        set(MK_FORMATS VST3 Standalone)
    endif()

    juce_add_plugin(MKWobbleBass
        COMPANY_NAME "MK Audio"
        BUNDLE_ID "audio.mk.wobblebass"
        PLUGIN_MANUFACTURER_CODE Mkau
        PLUGIN_CODE Mkwb
        PRODUCT_NAME "MK WOBBLE BASS"
        DESCRIPTION "Wobble bass synthesizer"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD ${MK_INSTALL_PLUGINS}
        NEEDS_WEB_BROWSER FALSE
        NEEDS_CURL FALSE
        MICROPHONE_PERMISSION_ENABLED FALSE
        BACKGROUND_AUDIO_ENABLED TRUE
        IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        VST3_CATEGORIES Instrument Synth
        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/AppIcon-1024.png"
        FORMATS ${MK_FORMATS})

    target_sources(MKWobbleBass PRIVATE
        Source/Plugin/PluginProcessor.cpp
        Source/Plugin/PluginEditor.cpp
        Source/UI/Theme.cpp
        Source/UI/GraphControls.cpp
        Source/UI/GatePattern.cpp
        Source/UI/PresetBrowser.cpp
        Source/UI/Pages.cpp)
    target_include_directories(MKWobbleBass PRIVATE Source)

    set(MK_PLUGIN_SOURCES
        Source/Plugin/PluginProcessor.cpp
        Source/Plugin/PluginEditor.cpp
        Source/UI/Theme.cpp
        Source/UI/GraphControls.cpp
        Source/UI/GatePattern.cpp
        Source/UI/PresetBrowser.cpp
        Source/UI/Pages.cpp)

    # Parameter ids are stable and the build has no VST2 predecessor, so the
    # VST2 migration guard is unnecessary; MIDI CC emulation parameters would
    # only bloat host automation with 1000+ unused entries.
    target_compile_definitions(MKWobbleBass PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_OPUS=0
        JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0)
    target_link_libraries(MKWobbleBass PRIVATE
        mkwobble_core
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

    juce_generate_juce_header(MKWobbleBass)

    if (MK_TARGET_IOS)
        set_target_properties(MKWobbleBass PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shared")
        set_target_properties(MKWobbleBass_Standalone MKWobbleBass_AUv3 PROPERTIES
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic)
    else()
        juce_add_console_app(MKPluginSmokeTest PRODUCT_NAME "MK Wobble Bass Smoke Test")
        target_sources(MKPluginSmokeTest PRIVATE Tools/PluginSmokeTest.cpp ${MK_PLUGIN_SOURCES})
        target_include_directories(MKPluginSmokeTest PRIVATE Source)
        target_compile_definitions(MKPluginSmokeTest PRIVATE
            JUCE_VST3_CAN_REPLACE_VST2=0
            MK_SMOKE_TEST=1)
        target_link_libraries(MKPluginSmokeTest PRIVATE
            mkwobble_core juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)

        # Off-screen renderer for the App Store / documentation assets.
        juce_add_console_app(MKThumbnail PRODUCT_NAME "MK Wobble Bass Thumbnail")
        target_sources(MKThumbnail PRIVATE Tools/MakeThumbnail.cpp ${MK_PLUGIN_SOURCES})
        target_include_directories(MKThumbnail PRIVATE Source)
        target_compile_definitions(MKThumbnail PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)
        target_link_libraries(MKThumbnail PRIVATE
            mkwobble_core juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
    endif()
endif()
