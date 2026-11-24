#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <array>
#include <cmath>
#include <memory>

namespace auren
{
constexpr int kBankCount = 4;
constexpr int kPadsPerBank = 16;
constexpr int kTotalPads = kBankCount * kPadsPerBank;
constexpr int kFirstPadMidiNote = 36; // C1 in the common drum-rack convention.

enum class DrumType : int
{
    kick,
    snare,
    clap,
    hat,
    openHat,
    tom,
    percussion,
    cymbal,
    sample
};

enum class OutputFamily : int
{
    main = 0,
    kick,
    snare,
    clap,
    hats,
    tom,
    percussion,
    cymbal
};

inline juce::String drumTypeName (DrumType type)
{
    switch (type)
    {
        case DrumType::kick:       return "Kick";
        case DrumType::snare:      return "Snare";
        case DrumType::clap:       return "Clap";
        case DrumType::hat:        return "Hat";
        case DrumType::openHat:    return "Open Hat";
        case DrumType::tom:        return "Tom";
        case DrumType::percussion: return "Perc";
        case DrumType::cymbal:     return "Cymbal";
        case DrumType::sample:     return "Sample";
    }

    return "Sample";
}

inline OutputFamily defaultOutputFor (DrumType type)
{
    switch (type)
    {
        case DrumType::kick:       return OutputFamily::kick;
        case DrumType::snare:      return OutputFamily::snare;
        case DrumType::clap:       return OutputFamily::clap;
        case DrumType::hat:
        case DrumType::openHat:    return OutputFamily::hats;
        case DrumType::tom:        return OutputFamily::tom;
        case DrumType::percussion: return OutputFamily::percussion;
        case DrumType::cymbal:     return OutputFamily::cymbal;
        case DrumType::sample:     return OutputFamily::main;
    }

    return OutputFamily::main;
}

struct PadParameters
{
    float clean = 1.0f;       // Original/isolated blend when both buffers exist.
    float gainDb = 0.0f;
    float pan = 0.0f;         // -1 = left, +1 = right.
    float tuneSemitones = 0.0f;
    float attackMs = 0.0f;
    float fadeMs = 30.0f;     // Tail fade duration.
    float highPassHz = 20.0f;
    float lowPassHz = 20000.0f;
    int chokeGroup = 0;
    bool muted = false;
    bool soloed = false;
    bool sendToMain = true;
    OutputFamily outputFamily = OutputFamily::main;

    void sanitise() noexcept
    {
        clean = juce::jlimit (0.0f, 1.0f, clean);
        gainDb = juce::jlimit (-60.0f, 18.0f, gainDb);
        pan = juce::jlimit (-1.0f, 1.0f, pan);
        tuneSemitones = juce::jlimit (-24.0f, 24.0f, tuneSemitones);
        attackMs = juce::jlimit (0.0f, 5000.0f, attackMs);
        fadeMs = juce::jlimit (0.0f, 10000.0f, fadeMs);
        highPassHz = juce::jlimit (5.0f, 18000.0f, highPassHz);
        lowPassHz = juce::jlimit (20.0f, 22000.0f, lowPassHz);
        if (highPassHz >= lowPassHz)
            highPassHz = juce::jmax (5.0f, lowPassHz * 0.5f);
        chokeGroup = juce::jlimit (0, 4, chokeGroup);
    }
};

struct PadSample
{
    juce::AudioBuffer<float> original;
    juce::AudioBuffer<float> isolated;
    double sourceSampleRate = 44100.0;
    juce::String name;
    juce::String sourcePath;
    DrumType type = DrumType::sample;
    int midiNote = kFirstPadMidiNote;

    [[nodiscard]] bool hasOriginal() const noexcept
    {
        return original.getNumChannels() > 0 && original.getNumSamples() > 0;
    }

    [[nodiscard]] bool hasIsolated() const noexcept
    {
        return isolated.getNumChannels() > 0 && isolated.getNumSamples() > 0;
    }

    [[nodiscard]] int getLengthInSamples() const noexcept
    {
        return juce::jmax (original.getNumSamples(), isolated.getNumSamples());
    }
};

inline int flatPadIndex (int bank, int pad) noexcept
{
    return juce::jlimit (0, kBankCount - 1, bank) * kPadsPerBank
         + juce::jlimit (0, kPadsPerBank - 1, pad);
}

inline int bankForFlatIndex (int index) noexcept
{
    return juce::jlimit (0, kTotalPads - 1, index) / kPadsPerBank;
}

inline int padForFlatIndex (int index) noexcept
{
    return juce::jlimit (0, kTotalPads - 1, index) % kPadsPerBank;
}
} // namespace auren
