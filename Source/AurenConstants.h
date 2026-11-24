#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace auren
{
inline constexpr int bankCount = 4;
inline constexpr int padsPerBank = 16;
inline constexpr int totalPadCount = bankCount * padsPerBank;
inline constexpr int baseMidiNote = 36;
inline constexpr int designWidth = 1584;
inline constexpr int designHeight = 992;
inline constexpr float outputHeadroomDb = -1.0f;

namespace ids
{
inline constexpr auto skin = "skin";
inline constexpr auto masterGain = "masterGain";
inline constexpr auto sensitivity = "sensitivity";
inline constexpr auto aiSeparation = "aiSeparation";
inline constexpr auto selectedBank = "selectedBank";
inline constexpr auto selectedPad = "selectedPad";
inline constexpr auto bpm = "bpm";
inline constexpr auto patternBars = "patternBars";
inline constexpr auto patternDivision = "patternDivision";
inline constexpr auto variationAmount = "variationAmount";
inline constexpr auto clean = "clean";
inline constexpr auto gain = "gain";
inline constexpr auto pan = "pan";
inline constexpr auto tune = "tune";
inline constexpr auto attack = "attack";
inline constexpr auto fade = "fade";
inline constexpr auto highPass = "highPass";
inline constexpr auto lowPass = "lowPass";
inline constexpr auto choke = "choke";
inline constexpr auto mute = "mute";
inline constexpr auto solo = "solo";
inline constexpr auto padOutput = "padOutput";
}

inline juce::String indexedParameterId (const char* base, int padIndex)
{
    return juce::String (base) + "_" + juce::String (padIndex);
}
}
