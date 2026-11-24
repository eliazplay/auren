#include "PadGrid.h"

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

PadGrid::PadGrid()
{
    addAndMakeVisible (banks);
    banks.onSelectionChange = [this] (int value) { setBank (value); };
    for (int i = 0; i < static_cast<int> (pads.size()); ++i)
    {
        pads[static_cast<size_t> (i)].name = "Pad " + juce::String (i + 1);
        pads[static_cast<size_t> (i)].midiNote = juce::MidiMessage::getMidiNoteName (36 + i, true, true, 3);
    }
}

void PadGrid::setTheme (Theme* newTheme) { theme = newTheme; banks.setTheme (theme); repaint(); }
void PadGrid::setPads (std::array<PadVisualState, 64> newPads) { pads = std::move (newPads); repaint(); }
void PadGrid::setPad (int index, PadVisualState state) { if (juce::isPositiveAndBelow (index, 64)) { pads[static_cast<size_t> (index)] = std::move (state); repaint(); } }
const PadVisualState& PadGrid::getPad (int index) const { return pads[static_cast<size_t> (juce::jlimit (0, 63, index))]; }

void PadGrid::setBank (int bank, juce::NotificationType notification)
{
    bank = juce::jlimit (0, 3, bank);
    if (currentBank == bank) return;
    currentBank = bank;
    banks.setSelectedIndex (bank, juce::dontSendNotification);
    selectedPad = currentBank * 16;
    repaint();
    if (notification != juce::dontSendNotification)
    {
        if (onBankChanged) onBankChanged (bank);
        if (onPadSelected) onPadSelected (selectedPad);
    }
}

void PadGrid::setSelectedPad (int index, juce::NotificationType notification)
{
    index = juce::jlimit (0, 63, index);
    selectedPad = index;
    const auto bank = index / 16;
    if (bank != currentBank)
    {
        currentBank = bank;
        banks.setSelectedIndex (bank, juce::dontSendNotification);
    }
    repaint();
    if (notification != juce::dontSendNotification && onPadSelected) onPadSelected (index);
}

juce::Rectangle<float> PadGrid::padBounds (int localIndex) const
{
    auto area = getLocalBounds().toFloat().withTrimmedTop (52.0f).reduced (8.0f, 5.0f);
    const auto gap = 7.0f;
    const auto width = (area.getWidth() - gap * 3.0f) / 4.0f;
    const auto height = (area.getHeight() - gap * 3.0f) / 4.0f;
    const auto row = localIndex / 4;
    const auto column = localIndex % 4;
    return { area.getX() + static_cast<float> (column) * (width + gap),
             area.getY() + static_cast<float> (row) * (height + gap), width, height };
}

void PadGrid::paint (juce::Graphics& g)
{
    const auto& c = paletteFor (theme);
    auto area = getLocalBounds().toFloat();
    Theme::drawRoundedPanel (g, area.reduced (1.0f), c.surface, c.line.withAlpha (0.52f), 12.0f, 1.0f);
    for (int local = 0; local < 16; ++local)
    {
        const auto index = currentBank * 16 + local;
        const auto& pad = pads[static_cast<size_t> (index)];
        auto rect = padBounds (local);
        const bool selected = index == selectedPad;
        const auto colour = theme != nullptr ? theme->familyColour (pad.family) : c.forestBright;
        g.setColour (c.shadow.withAlpha (0.3f));
        g.fillRoundedRectangle (rect.translated (0.0f, 2.0f), 9.0f);
        auto fill = pad.occupied ? c.panelDarkAlt : c.surfaceRaised;
        Theme::drawRoundedPanel (g, rect, fill,
                                 selected ? c.brass : c.line.withAlpha (0.55f), 9.0f,
                                 selected ? 2.2f : 1.0f);
        if (pad.occupied)
        {
            auto waveformArea = rect.reduced (9.0f).withTrimmedTop (18.0f);
            drawWaveform (g, waveformArea, pad.waveform, { 0.0f, 1.0f }, colour, true);
            if (pad.muted)
            {
                g.setColour (juce::Colours::black.withAlpha (0.6f));
                g.fillRoundedRectangle (rect, 9.0f);
            }
        }
        else
        {
            g.setColour (c.textMuted.withAlpha (0.62f));
            g.setFont (juce::Font (22.0f, juce::Font::plain));
            g.drawText ("+", rect, juce::Justification::centred);
        }
        g.setFont (juce::Font (10.5f, juce::Font::bold));
        g.setColour (pad.occupied ? juce::Colours::white.withAlpha (0.93f) : c.textMuted);
        auto top = rect.reduced (9.0f).removeFromTop (17.0f);
        if (pad.occupied)
        {
            g.setColour (colour);
            g.fillEllipse (top.getX(), top.getCentreY() - 3.0f, 6.0f, 6.0f);
            top.removeFromLeft (10.0f);
        }
        g.setColour (pad.occupied ? juce::Colours::white.withAlpha (0.93f) : c.textMuted);
        g.drawFittedText (pad.name.toUpperCase(), top.toNearestInt(), juce::Justification::centredLeft, 1);
        g.setFont (juce::Font (9.0f));
        g.setColour (pad.occupied ? juce::Colours::white.withAlpha (0.45f) : c.textMuted.withAlpha (0.65f));
        g.drawText (pad.midiNote, rect.reduced (8.0f).removeFromTop (16.0f), juce::Justification::centredRight);
        if (pad.solo)
        {
            g.setColour (c.brass);
            g.fillEllipse (rect.getRight() - 13.0f, rect.getBottom() - 13.0f, 6.0f, 6.0f);
        }
    }
}

void PadGrid::resized() { banks.setBounds (getLocalBounds().reduced (8, 6).removeFromTop (38)); }

void PadGrid::mouseDown (const juce::MouseEvent& event)
{
    for (int local = 0; local < 16; ++local)
    {
        if (! padBounds (local).contains (event.position)) continue;
        const auto index = currentBank * 16 + local;
        setSelectedPad (index);
        if (pads[static_cast<size_t> (index)].occupied && onPadTriggered)
            onPadTriggered (index, juce::jlimit (0.1f, 1.0f,
                            1.0f - (event.position.y - padBounds (local).getY()) / padBounds (local).getHeight()));
        break;
    }
}
} // namespace auren::ui
