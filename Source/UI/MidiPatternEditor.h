#pragma once

#include "PadGrid.h"

namespace auren::ui
{
struct MidiLane
{
    int padIndex = 0;
    juce::String name;
    juce::String family;
};

struct GridNote
{
    int padIndex = 0;
    double beat = 0.0;
    double length = 0.25;
    float velocity = 0.85f;
};

class MidiPatternEditor final : public juce::Component, public ThemedComponent
{
public:
    MidiPatternEditor();

    void setTheme (Theme*) override;
    void setPattern (std::vector<MidiLane>, std::vector<GridNote>, int bars);
    void setPatternIndex (int current, int total);
    void setGridDivision (int stepsPerQuarter);
    [[nodiscard]] const std::vector<GridNote>& getNotes() const noexcept { return notes; }

    std::function<void()> onDone;
    std::function<void(const std::vector<GridNote>&)> onNotesChanged;
    std::function<void(juce::String target, float amount)> onVary;
    std::function<void()> onUndo;
    std::function<void()> onRedo;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    [[nodiscard]] juce::Rectangle<float> gridBounds() const;
    void toggleNoteAt (juce::Point<float>, bool forceAdd);
    void applyThemeToChildren();

    Theme* theme = nullptr;
    std::vector<MidiLane> lanes;
    std::vector<GridNote> notes;
    int bars = 2;
    int patternIndex = 1;
    int patternTotal = 1;
    int stepsPerQuarter = 4;
    int lastPaintedCell = -1;

    AurenButton previous { "<" }, next { ">" }, done { "Done" };
    AurenButton original { "Original" }, undo { "Undo" }, redo { "Redo" };
    SegmentedControl quantize { { "1/8", "1/16", "1/32", "1/64" } };
    SegmentedControl varyTarget { { "AI", "Kick", "Snare", "Hats" } };
    SegmentedControl rolls { { "1/32", "1/32T", "1/64" } };
    AurenSlider varyAmount { "Subtle                         Bold", 0.45 };
};
} // namespace auren::ui
