#pragma once

#include "MidiPatternEditor.h"

namespace auren::ui
{
class AurenMainView final : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void()> importAudio, browseAudio, analyseWithAI, reclone, saveKit, loadPad;
        std::function<void(int)> selectPad;
        std::function<void(int, float)> triggerPad;
        std::function<void(int, const juce::String&, double)> setPadParameter;
        std::function<void(const juce::String&)> assistantMessage;
        std::function<void(bool)> playPattern;
        std::function<void(const std::vector<GridNote>&)> patternEdited;
    };

    AurenMainView();

    void setSkin (Skin);
    [[nodiscard]] Skin getSkin() const noexcept { return theme.getSkin(); }
    void setCallbacks (Callbacks callbacksToUse) { callbacks = std::move (callbacksToUse); }
    void showMidiEditor (bool shouldShow);
    [[nodiscard]] bool isMidiEditorVisible() const noexcept { return midiMode; }

    void setSourceAudio (std::vector<float>, std::vector<HitMarker>, std::vector<FamilyCount>,
                         juce::String fileName, juce::String duration, int hitCount);
    void setPads (std::array<PadVisualState, 64>);
    void setSelectedPadAudio (int padIndex, juce::String name, juce::String family,
                              std::vector<float>, int takeIndex, int takeCount);
    void setMidiPattern (std::vector<MidiLane>, std::vector<GridNote>, int bars,
                         int patternIndex, int patternTotal);
    void setAssistantMessage (juce::String);

    MidiPatternEditor& getMidiEditor() noexcept { return midiEditor; }
    PadGrid& getPadGrid() noexcept { return pads; }
    SourceWaveformView& getSourceWaveform() noexcept { return source; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void applyTheme();
    void wireCallbacks();
    void updateVisibility();
    juce::Rectangle<int> innerBounds() const;

    Theme theme;
    RackFrame rack { &theme };
    Callbacks callbacks;
    bool midiMode = false;
    bool playing = false;
    int selectedPadIndex = 0;
    juce::String selectedPadName { "Pad 1" };
    juce::String selectedPadFamily { "Sample" };
    int selectedTake = 0, takeCount = 0;

    AurenButton previousKit { "<" }, nextKit { ">" }, save { "Save" }, settings { "Settings" };
    AurenSlider masterOut { "OUT", 0.78 };
    AurenButton ai { "AI" }, reclone { "Re-clone" }, browse { "Browse" }, importAudio { "Import" };
    AurenSlider sensitivity { "SENSITIVITY", 0.55 };
    SourceWaveformView source;
    AssistantCard assistant;
    PadGrid pads;
    PadWaveformView padWaveform;
    AurenButton padType { "Pad Type" }, mute { "M" }, solo { "S" }, load { "Load" };
    AurenButton previousTake { "<" }, nextTake { ">" };
    std::array<std::unique_ptr<AurenKnob>, 8> knobs;
    SegmentedControl choke { { "Off", "1", "2", "3", "4" } };
    AurenButton output { "Main + Pad" };

    AurenButton editMidi { "Pencil" }, play { "Play" }, stop { "Stop" };
    AurenButton bpm { "120 BPM" }, pattern { "Pattern 1" }, barCount { "2 bars" }, dragMidi { "Drag into your DAW" };
    MidiPatternEditor midiEditor;
};
} // namespace auren::ui
