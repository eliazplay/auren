#pragma once

#include <JuceHeader.h>
#include "AurenProcessor.h"
#include "../UI/AurenMainView.h"

namespace auren
{
class AurenEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AurenEditor (::AurenProcessor&);
    ~AurenEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void refresh();
    void chooseSource();
    void choosePad();
    static std::vector<float> mono (const juce::AudioBuffer<float>&);
    ::AurenProcessor& processor;
    ui::AurenMainView view;
    std::unique_ptr<juce::FileChooser> chooser;
};
}
