cmake_minimum_required(VERSION 3.24)

project(AUREN
    VERSION 0.1.0
    DESCRIPTION "AUREN drum extraction and performance instrument"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AUREN_BUILD_STANDALONE "Build the standalone AUREN application" ON)

include(FetchContent)

# JUCE 8.0.15, pinned to the commit behind the upstream release tag so a tag
# move cannot silently change production builds.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 91ad83ae34a81e0833b1a2b0866f54846370ae53
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)

include(cmake/AurenCompiler.cmake)

set(AUREN_FORMATS VST3)
if (AUREN_BUILD_STANDALONE)
    list(APPEND AUREN_FORMATS Standalone)
endif()

juce_add_plugin(AUREN
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "AUREN Audio"
    BUNDLE_ID "audio.auren.instrument"
    PLUGIN_MANUFACTURER_CODE Arna
    PLUGIN_CODE Aurn
    FORMATS ${AUREN_FORMATS}
    PRODUCT_NAME "AUREN"
    DESCRIPTION "AUREN drum extraction and performance instrument"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Sampler Drum
    COPY_PLUGIN_AFTER_BUILD FALSE
)

file(GLOB_RECURSE AUREN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

if (NOT AUREN_SOURCES)
    message(FATAL_ERROR "No AUREN sources were found under Source/")
endif()

target_sources(AUREN PRIVATE ${AUREN_SOURCES})
target_include_directories(AUREN PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_compile_definitions(AUREN
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
    PRIVATE
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(AUREN
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_data_structures
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# The user interface is rendered from the design canvas rather than native
# controls. Vector drawing keeps the rack face visually identical on every
# Windows host and at every supported DPI scale.
target_compile_definitions(AUREN PRIVATE JUCE_USE_WINRT_MIDI=0)

auren_apply_project_options(AUREN)

juce_generate_juce_header(AUREN)

