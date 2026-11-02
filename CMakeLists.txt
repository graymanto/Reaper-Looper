cmake_minimum_required(VERSION 3.22)

project(ReaperLooper VERSION 0.1.0 LANGUAGES C CXX)

# ---- C++20 ------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_policy(SET CMP0135 NEW)

# ---- Platform deployment target ---------------------------------------------
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.4" CACHE STRING "Minimum macOS deployment target")
endif()

# ---- Dependencies ------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# ---- Core library (shared between plugin and tests) -------------------------
# Framework-agnostic looper engine. Links only juce_core/juce_audio_basics/
# juce_data_structures - no GUI, no plugin client, no REAPER API calls (those
# live in source/reaper/, which depends on juce_audio_processors for
# VST3ClientExtensions). This mirrors the Core/UI split used in `sequenced`.
add_library(ReaperLooperCore STATIC
    source/core/Constants.h
    source/core/model/MidiNote.h
    source/core/model/ClipTypes.h
    source/core/model/Clip.h
    source/core/model/Clip.cpp
    source/core/model/MidiClip.h
    source/core/model/MidiClip.cpp
    source/core/model/ClipFactory.h
    source/core/model/ClipFactory.cpp
    source/core/engine/HostTransportState.h
    source/core/engine/LaunchQuantizer.h
    source/core/engine/LaunchQuantizer.cpp
    source/core/engine/LoopSegmenter.h
    source/core/engine/LoopSegmenter.cpp
    source/core/engine/RecordBuffer.h
    source/core/engine/RecordBuffer.cpp
    source/core/session/Track.h
    source/core/session/Track.cpp
    source/core/session/Session.h
    source/core/session/Session.cpp
    source/core/session/ProjectSerializer.h
    source/core/session/ProjectSerializer.cpp
    source/core/controller/GridViewport.h
    source/core/controller/LaunchpadProfile.h
    source/core/controller/LaunchpadProfile.cpp
)

target_compile_features(ReaperLooperCore PUBLIC cxx_std_20)
target_include_directories(ReaperLooperCore PUBLIC source)
target_compile_definitions(ReaperLooperCore PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

target_link_libraries(ReaperLooperCore
    PUBLIC
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_data_structures
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

set_target_properties(ReaperLooperCore PROPERTIES LINKER_LANGUAGE CXX)

# ---- REAPER adapter library ---------------------------------------------------
# The REAPER-specific layer: acquiring the REAPER C++ extension API from
# inside a VST3 instance (ReaperApi), plus (in later phases) track
# creation/routing, transport/tempo bridging, and undo blocks. Needs
# juce_audio_processors for VST3ClientExtensions, so it is a separate target
# from ReaperLooperCore rather than folded into it.
add_library(ReaperLooperReaperAdapter STATIC
    source/reaper/ReaperApi.h
    source/reaper/ReaperApi.cpp
    source/reaper/ReaperApiWantList.h
    source/reaper/ReaperFunctions.h
    source/reaper/ReaperFunctions.cpp
    source/reaper/TransportBridge.h
    source/reaper/TransportBridge.cpp
    source/reaper/TrackRegistry.h
    source/reaper/TrackRegistry.cpp
    source/reaper/TrackRouter.h
    source/reaper/TrackRouter.cpp
    source/reaper/sdk/ReaperForwardDecls.h
    source/reaper/sdk/reaper_plugin_functions.h
    source/reaper/sdk/reaper_vst3_interfaces.h
)

target_compile_features(ReaperLooperReaperAdapter PUBLIC cxx_std_20)
target_include_directories(ReaperLooperReaperAdapter PUBLIC source)
target_compile_definitions(ReaperLooperReaperAdapter PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

target_link_libraries(ReaperLooperReaperAdapter
    PUBLIC
        ReaperLooperCore
        juce::juce_audio_processors
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ---- Plugin -------------------------------------------------------------------
set(REAPERLOOPER_PLUGIN_ARGS
    VERSION                     "${PROJECT_VERSION}"
    COMPANY_NAME                "ReaperLooper"
    PLUGIN_MANUFACTURER_CODE    "Rplp"
    PLUGIN_CODE                 "Rlp1"
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "ReaperLooper"
    BUNDLE_ID                   "com.reaperlooper.plugin"
    IS_MIDI_EFFECT               TRUE
    NEEDS_MIDI_INPUT             TRUE
    NEEDS_MIDI_OUTPUT            TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS  TRUE
    COPY_PLUGIN_AFTER_BUILD      TRUE
    VST3_CATEGORIES              "Instrument" "Fx" "Midi"
    MICROPHONE_PERMISSION_ENABLED FALSE
)

juce_add_plugin(ReaperLooper
    ${REAPERLOOPER_PLUGIN_ARGS}
)

juce_generate_juce_header(ReaperLooper)

target_sources(ReaperLooper
    PRIVATE
        source/plugin/PluginProcessor.cpp
        source/plugin/PluginEditor.cpp
        source/plugin/LaunchpadController.cpp
        source/ui/ReaperLooperLookAndFeel.cpp
        source/ui/grid/ClipCell.cpp
        source/ui/grid/GridView.cpp
        source/ui/transport/TransportBar.cpp
)

target_include_directories(ReaperLooper
    PRIVATE
        source
)

target_compile_definitions(ReaperLooper
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(ReaperLooper
    PRIVATE
        ReaperLooperCore
        ReaperLooperReaperAdapter
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ---- Tests ----------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
