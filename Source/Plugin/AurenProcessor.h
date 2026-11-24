#pragma once

#include <JuceHeader.h>
#include "../AurenConstants.h"
#include "../Assistant/AurenAssistant.h"
#include "../DSP/CleanHitExtractor.h"
#include "../DSP/DrumAnalysis.h"
#include "../Engine/SamplerEngine.h"

namespace auren { class AurenEditor; }

class AurenProcessor final : public juce::AudioProcessor
{
public:
    AurenProcessor();
    ~AurenProcessor() override = default;
    void prepareToPlay (double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AUREN"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; } int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {} const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    auren::AurenModel& model() noexcept { return kitModel; }
    const auren::AurenModel& model() const noexcept { return kitModel; }
    auren::AurenAssistant& assistant() noexcept { return aiAssistant; }
    juce::AudioProcessorValueTreeState& parameters() noexcept { return apvts; }
    const juce::AudioBuffer<float>& sourceAudio() const noexcept { return source; }
    const std::vector<auren::dsp::HitDescriptor>& sourceHits() const noexcept { return hits; }
    void loadSource (const juce::File&);
    void cloneSource();
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
private:
    auren::AurenModel kitModel;
    auren::SamplerEngine sampler { kitModel };
    auren::AurenAssistant aiAssistant;
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioFormatManager formats;
    juce::AudioBuffer<float> source;
    std::vector<auren::dsp::HitDescriptor> hits;
    double sourceRate = 44100.0;
};
