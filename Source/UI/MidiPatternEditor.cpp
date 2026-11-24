#include "MidiPatternEditor.h"

namespace auren::ui
{
namespace
{
const Palette& paletteFor (Theme* theme)
{
    static Theme fallback;
    return theme != nullptr ? theme->colours() : fallback.colours();
}
}

MidiPatternEditor::MidiPatternEditor()
{
    for (auto* component : std::array<juce::Component*, 10> { &previous, &next, &done, &original,
                                                              &undo, &redo, &quantize, &varyTarget,
                                                              &rolls, &varyAmount })
        addAndMakeVisible (component);
    done.setAccent (true);
    original.setAccent (true);
    quantize.setSelectedIndex (1, juce::dontSendNotification);
    quantize.onSelectionChange = [this] (int index)
    {
        constexpr std::array<int, 4> divisions { 2, 4, 8, 16 };
        setGridDivision (divisions[static_cast<size_t> (index)]);
    };
    done.onClick = [this] { if (onDone) onDone(); };
    undo.onClick = [this] { if (onUndo) onUndo(); };
    redo.onClick = [this] { if (onRedo) onRedo(); };
    original.onClick = [this] { if (onVary) onVary ("Original", 0.0f); };
    varyTarget.onSelectionChange = [this] (int index)
    {
        static const juce::StringArray names { "AI", "Kick", "Snare", "Hats" };
        if (onVary) onVary (names[index], static_cast<float> (varyAmount.getValue()));
    };
}

void MidiPatternEditor::setTheme (Theme* newTheme) { theme = newTheme; applyThemeToChildren(); repaint(); }

void MidiPatternEditor::applyThemeToChildren()
{
    for (auto* component : std::array<ThemedComponent*, 10> { &previous, &next, &done, &original,
                                                              &undo, &redo, &quantize, &varyTarget,
                                                              &rolls, &varyAmount })
        component->setTheme (theme);
}

void MidiPatternEditor::setPattern (std::vector<MidiLane> newLanes, std::vector<GridNote> newNotes, int newBars)
{
    lanes = std::move (newLanes); notes = std::move (newNotes); bars = juce::jlimit (1, 8, newBars); repaint();
}

void MidiPatternEditor::setPatternIndex (int current, int total)
{
    patternTotal = juce::jmax (1, total); patternIndex = juce::jlimit (1, patternTotal, current); repaint();
}

void MidiPatternEditor::setGridDivision (int division) { stepsPerQuarter = juce::jlimit (1, 16, division); repaint(); }

juce::Rectangle<float> MidiPatternEditor::gridBounds() const
{
    return getLocalBounds().toFloat().reduced (26.0f, 18.0f).withTrimmedTop (220.0f).withTrimmedBottom (10.0f);
}

void MidiPatternEditor::paint (juce::Graphics& g)
{
    const auto& c = paletteFor (theme);
    auto area = getLocalBounds().toFloat();
    Theme::drawRoundedPanel (g, area.reduced (1.0f), c.panelDark, c.line.withAlpha (0.65f), 12.0f, 1.0f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.drawText ("PATTERN", 28, 20, 90, 28, juce::Justification::centredLeft);
    g.drawText ("Pattern " + juce::String (patternIndex) + " of " + juce::String (patternTotal),
                250, 20, 190, 28, juce::Justification::centred);
    g.drawText (juce::String (bars) + " bars", 520, 20, 100, 28, juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("VARY", 28, 78, 75, 28, juce::Justification::centredLeft);
    g.drawText ("ROLLS", 28, 133, 75, 28, juce::Justification::centredLeft);

    auto grid = gridBounds();
    const auto labelWidth = juce::jmin (190.0f, grid.getWidth() * 0.16f);
    auto labels = grid.removeFromLeft (labelWidth);
    const int laneCount = juce::jmax (1, static_cast<int> (lanes.size()));
    const auto laneHeight = grid.getHeight() / static_cast<float> (laneCount);
    const auto totalSteps = juce::jmax (1, bars * 4 * stepsPerQuarter);
    const auto stepWidth = grid.getWidth() / static_cast<float> (totalSteps);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRect (labels);
    for (int lane = 0; lane <= laneCount; ++lane)
    {
        const auto y = grid.getY() + laneHeight * static_cast<float> (lane);
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawHorizontalLine (juce::roundToInt (y), labels.getX(), grid.getRight());
    }
    for (int step = 0; step <= totalSteps; ++step)
    {
        const auto x = grid.getX() + stepWidth * static_cast<float> (step);
        const bool barLine = step % (stepsPerQuarter * 4) == 0;
        const bool beatLine = step % stepsPerQuarter == 0;
        g.setColour (juce::Colours::white.withAlpha (barLine ? 0.42f : beatLine ? 0.2f : 0.07f));
        g.drawVerticalLine (juce::roundToInt (x), grid.getY(), grid.getBottom());
    }
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    for (int lane = 0; lane < static_cast<int> (lanes.size()); ++lane)
    {
        const auto& item = lanes[static_cast<size_t> (lane)];
        auto laneRect = juce::Rectangle<float> (labels.getX(), grid.getY() + laneHeight * lane,
                                                labels.getWidth(), laneHeight);
        const auto colour = theme != nullptr ? theme->familyColour (item.family) : c.forestBright;
        g.setColour (colour); g.fillEllipse (laneRect.getX() + 12.0f, laneRect.getCentreY() - 4.0f, 8.0f, 8.0f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawFittedText (item.name, laneRect.toNearestInt().withTrimmedLeft (30), juce::Justification::centredLeft, 1);
    }
    for (const auto& note : notes)
    {
        const auto lane = std::find_if (lanes.begin(), lanes.end(), [&note] (const MidiLane& item) { return item.padIndex == note.padIndex; });
        if (lane == lanes.end()) continue;
        const auto laneIndex = static_cast<int> (std::distance (lanes.begin(), lane));
        const auto colour = theme != nullptr ? theme->familyColour (lane->family) : c.forestBright;
        const auto x = grid.getX() + static_cast<float> (note.beat / static_cast<double> (bars * 4)) * grid.getWidth();
        const auto width = juce::jmax (5.0f, static_cast<float> (note.length / static_cast<double> (bars * 4)) * grid.getWidth());
        auto noteRect = juce::Rectangle<float> (x + 1.0f, grid.getY() + laneHeight * laneIndex + 3.0f,
                                                width - 2.0f, laneHeight - 6.0f);
        g.setColour (colour.withMultipliedBrightness (0.72f + note.velocity * 0.34f));
        g.fillRoundedRectangle (noteRect, 3.0f);
    }
}

void MidiPatternEditor::resized()
{
    previous.setBounds (160, 20, 38, 30); next.setBounds (202, 20, 38, 30);
    quantize.setBounds (670, 20, 420, 32); done.setBounds (getWidth() - 135, 20, 105, 32);
    varyTarget.setBounds (120, 76, 460, 34); varyAmount.setBounds (650, 76, getWidth() - 690, 34);
    rolls.setBounds (120, 131, 300, 34); original.setBounds (28, 180, 110, 30);
    undo.setBounds (getWidth() - 160, 180, 58, 30); redo.setBounds (getWidth() - 95, 180, 58, 30);
}

void MidiPatternEditor::mouseDown (const juce::MouseEvent& event) { lastPaintedCell = -1; toggleNoteAt (event.position, event.mods.isShiftDown()); }
void MidiPatternEditor::mouseDrag (const juce::MouseEvent& event) { toggleNoteAt (event.position, true); }

void MidiPatternEditor::toggleNoteAt (juce::Point<float> point, bool forceAdd)
{
    auto grid = gridBounds();
    const auto labelWidth = juce::jmin (190.0f, grid.getWidth() * 0.16f);
    grid.removeFromLeft (labelWidth);
    if (! grid.contains (point) || lanes.empty()) return;
    const auto laneIndex = juce::jlimit (0, static_cast<int> (lanes.size()) - 1,
                                        static_cast<int> ((point.y - grid.getY()) / grid.getHeight() * lanes.size()));
    const auto totalSteps = bars * 4 * stepsPerQuarter;
    const auto step = juce::jlimit (0, totalSteps - 1,
                                    static_cast<int> ((point.x - grid.getX()) / grid.getWidth() * totalSteps));
    const auto cell = laneIndex * totalSteps + step;
    if (cell == lastPaintedCell) return;
    lastPaintedCell = cell;
    const auto padIndex = lanes[static_cast<size_t> (laneIndex)].padIndex;
    const auto beat = static_cast<double> (step) / static_cast<double> (stepsPerQuarter);
    auto found = std::find_if (notes.begin(), notes.end(), [=] (const GridNote& note)
    {
        return note.padIndex == padIndex && std::abs (note.beat - beat) < 0.0001;
    });
    if (found != notes.end())
    {
        if (! forceAdd) notes.erase (found);
    }
    else
    {
        notes.push_back ({ padIndex, beat, 1.0 / static_cast<double> (stepsPerQuarter),
                           juce::jlimit (0.15f, 1.0f, 1.0f - (point.y - (grid.getY() + laneIndex * grid.getHeight() / lanes.size())) /
                                                        (grid.getHeight() / lanes.size())) });
    }
    repaint();
    if (onNotesChanged) onNotesChanged (notes);
}
} // namespace auren::ui
