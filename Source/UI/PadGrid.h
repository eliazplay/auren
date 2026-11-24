#pragma once

#include "WaveformViews.h"

namespace auren::ui
{
struct PadVisualState
{
    juce::String name;
    juce::String family;
    juce::String midiNote;
    std::vector<float> waveform;
    bool occupied = false;
    bool muted = false;
    bool solo = false;
};

class PadGrid final : public juce::Component, public ThemedComponent
{
public:
    PadGrid();

    void setTheme (Theme*) override;
    void setPads (std::array<PadVisualState, 64>);
    void setPad (int globalIndex, PadVisualState);
    [[nodiscard]] const PadVisualState& getPad (int globalIndex) const;
    void setBank (int bank, juce::NotificationType = juce::sendNotificationAsync);
    [[nodiscard]] int getBank() const noexcept { return currentBank; }
    void setSelectedPad (int globalIndex, juce::NotificationType = juce::sendNotificationAsync);
    [[nodiscard]] int getSelectedPad() const noexcept { return selectedPad; }

    std::function<void(int)> onPadSelected;
    std::function<void(int, float)> onPadTriggered;
    std::function<void(int)> onBankChanged;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    [[nodiscard]] juce::Rectangle<float> padBounds (int localIndex) const;
    Theme* theme = nullptr;
    std::array<PadVisualState, 64> pads;
    SegmentedControl banks { { "A", "B", "C", "D" } };
    int currentBank = 0;
    int selectedPad = 0;
};
} // namespace auren::ui
