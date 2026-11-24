#pragma once

#include "DrumTypes.h"

namespace auren::dsp
{
struct ExtractionOptions
{
    int maximumOccurrences = 32;
    float maximumLengthMs = 1800.0f;
    float alignmentWindowMs = 7.0f;
    float correlationWindowMs = 65.0f;
    float minimumCorrelation = 0.16f;
    float trimmedFraction = 0.18f;
    float cleanAmount = 0.78f;
    float targetPeakDb = -1.0f;
    float fadeOutMs = 12.0f;
    bool removeSubsonics = true;
    bool protectKickResonances = true;
};

struct ExtractionResult
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 0.0;
    DrumFamily family = DrumFamily::unknown;
    int usedHitCount = 0;
    float confidence = 0.0f;
    float detectedFundamentalHz = 0.0f;
    juce::String status;

    [[nodiscard]] bool isValid() const noexcept
    {
        return sampleRate > 0.0 && audio.getNumChannels() > 0 && audio.getNumSamples() > 0;
    }
};

class CleanHitExtractor
{
public:
    [[nodiscard]] static ExtractionResult extractGroup (
        const juce::AudioBuffer<float>& source,
        double sampleRate,
        const std::vector<HitDescriptor>& hits,
        int groupId,
        const ExtractionOptions& options = {});

    [[nodiscard]] static ExtractionResult extractSingle (
        const juce::AudioBuffer<float>& source,
        double sampleRate,
        const HitDescriptor& hit,
        const ExtractionOptions& options = {});
};
} // namespace auren::dsp
