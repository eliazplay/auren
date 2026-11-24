#pragma once

#include "AurenModel.h"

#include <array>

namespace auren
{
class SamplerEngine
{
public:
    static constexpr int kMaxVoices = 64;
    using FamilyOutputs = std::array<juce::AudioBuffer<float>*, 8>;

    explicit SamplerEngine (AurenModel& state) : model (state) {}
    void prepare (double sampleRate, int maximumBlockSize, int outputChannels);
    void reset() noexcept;
    void triggerPad (int bank, int padIndex, float velocity = 1.0f);
    void triggerFlatPad (int flatPad, float velocity = 1.0f);
    void chokeGroup (int group) noexcept;
    void processBlock (juce::AudioBuffer<float>& mainOutput,
                       const FamilyOutputs& familyOutputs,
                       const juce::MidiBuffer& midi,
                       int selectedBank);

private:
    struct OnePole
    {
        void reset() noexcept { lowState = highTrackingState = 0.0f; }
        void set (double sampleRate, float highPassHz, float lowPassHz) noexcept;
        float process (float input) noexcept;
        float lowCoefficient = 1.0f, highCoefficient = 1.0f;
        float lowState = 0.0f, highTrackingState = 0.0f;
    };

    struct Voice
    {
        bool active = false;
        int flatPad = 0, choke = 0;
        std::shared_ptr<const PadSample> sample;
        PadParameters parameters;
        double sourcePosition = 0.0, increment = 1.0;
        float velocity = 1.0f;
        int ageSamples = 0, releaseSamplesLeft = 0, releaseSamplesTotal = 1;
        std::array<OnePole, 2> filters;
        void release (int samples) noexcept { releaseSamplesTotal = juce::jmax (1, samples); releaseSamplesLeft = releaseSamplesTotal; }
    };

    Voice& findVoiceToUse() noexcept;
    void renderRange (juce::AudioBuffer<float>&, const FamilyOutputs&, int, int);
    float readSample (const juce::AudioBuffer<float>&, int, double) const noexcept;
    AurenModel& model;
    std::array<Voice, kMaxVoices> voices;
    double outputSampleRate = 44100.0;
    int numOutputChannels = 2;
};
} // namespace auren
