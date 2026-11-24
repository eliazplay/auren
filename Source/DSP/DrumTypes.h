#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cstdint>
#include <vector>

namespace auren::dsp
{
enum class DrumFamily
{
    kick,
    snare,
    clap,
    closedHat,
    openHat,
    tom,
    percussion,
    cymbal,
    unknown
};

inline juce::String familyName (DrumFamily family)
{
    switch (family)
    {
        case DrumFamily::kick:       return "Kick";
        case DrumFamily::snare:      return "Snare";
        case DrumFamily::clap:       return "Clap";
        case DrumFamily::closedHat:  return "Hat";
        case DrumFamily::openHat:    return "Open Hat";
        case DrumFamily::tom:        return "Tom";
        case DrumFamily::percussion: return "Perc";
        case DrumFamily::cymbal:     return "Cymbal";
        case DrumFamily::unknown:    break;
    }

    return "Sample";
}

// Features are deliberately compact so they can be saved with a kit without
// storing FFT data. Their order is documented here and kept stable.
enum FeatureIndex : std::size_t
{
    lowBandRatio,
    midBandRatio,
    highBandRatio,
    spectralCentroid,
    zeroCrossingRate,
    decayRatio,
    spectralFlatness,
    transientStrength,
    featureCount
};

struct HitDescriptor
{
    std::int64_t samplePosition = 0;
    float strength = 0.0f;
    DrumFamily family = DrumFamily::unknown;
    std::array<float, featureCount> features {};
    int groupId = -1;
};

struct AnalysisOptions
{
    // 0 detects only strong transients; 1 also keeps quiet ghost notes.
    float sensitivity = 0.62f;
    float minimumGapMs = 28.0f;
    int analysisFftSize = 1024;
    int hopSize = 256;
    int maxHits = 4096;
    float groupingDistance = 0.30f;
    int maximumGroupsPerFamily = 12;
};
} // namespace auren::dsp
