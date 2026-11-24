#include "AurenMainView.h"

namespace auren::ui
{
AurenMainView::AurenMainView()
{
    const std::array<juce::String, 8> names { "CLEAN", "GAIN", "PAN", "TUNE", "ATTACK", "FADE", "HP", "LP" };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto bipolar = i == 2 || i == 3;
        knobs[i] = std::make_unique<AurenKnob> (names[i],
                                                juce::NormalisableRange<double> (bipolar ? -1.0 : 0.0, 1.0, 0.001),
                                                bipolar ? 0.0 : (i < 2 ? 0.7 : 0.25));
    }

    addAndMakeVisible (rack);
    for (auto* component : std::array<juce::Component*, 31> {
             &previousKit, &nextKit, &save, &settings, &masterOut, &ai, &reclone, &browse,
             &importAudio, &sensitivity, &source, &assistant, &pads, &padWaveform, &padType,
             &mute, &solo, &load, &previousTake, &nextTake,
             knobs[0].get(), knobs[1].get(), knobs[2].get(), knobs[3].get(), knobs[4].get(),
             knobs[5].get(), knobs[6].get(), knobs[7].get(), &choke, &output, &midiEditor })
        addAndMakeVisible (component);
    for (auto* component : std::array<juce::Component*, 7> { &editMidi, &play, &stop, &bpm, &pattern, &barCount, &dragMidi })
        addAndMakeVisible (component);

    save.setAccent (true); ai.setAccent (true); importAudio.setAccent (true); load.setAccent (true);
    editMidi.setAccent (true); play.setAccent (true);
    mute.setClickingTogglesState (true); solo.setClickingTogglesState (true);
    setSize (1584, 991);
    applyTheme();
    wireCallbacks();
    updateVisibility();
}

void AurenMainView::setSkin (Skin skin) { theme.setSkin (skin); applyTheme(); repaint(); }

void AurenMainView::applyTheme()
{
    rack.setTheme (&theme); source.setTheme (&theme); assistant.setTheme (&theme); pads.setTheme (&theme);
    padWaveform.setTheme (&theme); choke.setTheme (&theme); sensitivity.setTheme (&theme); masterOut.setTheme (&theme);
    midiEditor.setTheme (&theme);
    for (auto* button : std::array<AurenButton*, 22> { &previousKit, &nextKit, &save, &settings, &ai,
            &reclone, &browse, &importAudio, &padType, &mute, &solo, &load, &previousTake, &nextTake,
            &output, &editMidi, &play, &stop, &bpm, &pattern, &barCount, &dragMidi })
        button->setTheme (&theme);
    for (auto& knob : knobs) knob->setTheme (&theme);
}

void AurenMainView::wireCallbacks()
{
    importAudio.onClick = [this] { if (callbacks.importAudio) callbacks.importAudio(); };
    browse.onClick = [this] { if (callbacks.browseAudio) callbacks.browseAudio(); };
    ai.onClick = [this] { if (callbacks.analyseWithAI) callbacks.analyseWithAI(); };
    reclone.onClick = [this] { if (callbacks.reclone) callbacks.reclone(); };
    save.onClick = [this] { if (callbacks.saveKit) callbacks.saveKit(); };
    load.onClick = [this] { if (callbacks.loadPad) callbacks.loadPad(); };
    assistant.onSubmit = [this] (const juce::String& text) { if (callbacks.assistantMessage) callbacks.assistantMessage (text); };
    pads.onPadSelected = [this] (int index) { selectedPadIndex = index; if (callbacks.selectPad) callbacks.selectPad (index); };
    pads.onPadTriggered = [this] (int index, float velocity) { if (callbacks.triggerPad) callbacks.triggerPad (index, velocity); };
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i]->onValueChange = [this, i] (double value)
        {
            if (callbacks.setPadParameter) callbacks.setPadParameter (selectedPadIndex, knobs[i]->getLabel(), value);
        };
    editMidi.onClick = [this] { showMidiEditor (true); };
    midiEditor.onDone = [this] { showMidiEditor (false); };
    midiEditor.onNotesChanged = [this] (const std::vector<GridNote>& notes) { if (callbacks.patternEdited) callbacks.patternEdited (notes); };
    play.onClick = [this] { playing = true; if (callbacks.playPattern) callbacks.playPattern (true); };
    stop.onClick = [this] { playing = false; if (callbacks.playPattern) callbacks.playPattern (false); };
}

void AurenMainView::showMidiEditor (bool shouldShow) { midiMode = shouldShow; updateVisibility(); resized(); repaint(); }

void AurenMainView::updateVisibility()
{
    for (auto* component : std::array<juce::Component*, 18> { &source, &sensitivity, &ai, &reclone, &browse,
             &importAudio, &assistant, &pads, &padWaveform, &padType, &mute, &solo, &load, &previousTake,
             &nextTake, &choke, &output, &masterOut })
        component->setVisible (! midiMode);
    for (auto& knob : knobs) knob->setVisible (! midiMode);
    midiEditor.setVisible (midiMode);
}

void AurenMainView::setSourceAudio (std::vector<float> audio, std::vector<HitMarker> markers,
                                    std::vector<FamilyCount> counts, juce::String fileName,
                                    juce::String duration, int hits)
{
    source.setAudio (std::move (audio)); source.setMarkers (std::move (markers));
    source.setFamilyCounts (std::move (counts)); source.setMetadata (std::move (fileName), std::move (duration), hits);
}

void AurenMainView::setPads (std::array<PadVisualState, 64> states) { pads.setPads (std::move (states)); }

void AurenMainView::setSelectedPadAudio (int index, juce::String name, juce::String family,
                                         std::vector<float> audio, int take, int total)
{
    selectedPadIndex = juce::jlimit (0, 63, index); selectedPadName = std::move (name);
    selectedPadFamily = std::move (family); selectedTake = take; takeCount = total;
    pads.setSelectedPad (selectedPadIndex, juce::dontSendNotification);
    padWaveform.setAudio (std::move (audio), selectedPadFamily); repaint();
}

void AurenMainView::setMidiPattern (std::vector<MidiLane> lanes, std::vector<GridNote> notes, int bars,
                                    int index, int total)
{
    midiEditor.setPattern (std::move (lanes), std::move (notes), bars);
    midiEditor.setPatternIndex (index, total);
    pattern.setButtonText ("Pattern " + juce::String (index));
    barCount.setButtonText (juce::String (bars) + " bars");
}

void AurenMainView::setAssistantMessage (juce::String text) { assistant.setMessage (std::move (text)); }

juce::Rectangle<int> AurenMainView::innerBounds() const
{
    return getLocalBounds().reduced (juce::roundToInt (Metrics::rackEar + 24.0f), 18);
}

void AurenMainView::paint (juce::Graphics& g)
{
    const auto& c = theme.colours();
    auto inner = innerBounds();
    auto header = inner.removeFromTop (58);
    Theme::drawWaveMark (g, header.removeFromLeft (64).toFloat().reduced (10.0f), c.forest);
    g.setColour (c.text); g.setFont (juce::Font (30.0f, juce::Font::bold));
    g.drawText ("AUREN", header.removeFromLeft (200), juce::Justification::centredLeft);
    if (! midiMode)
    {
        auto inspector = inner.withTrimmedTop (235).removeFromRight (inner.getWidth() * 34 / 100);
        Theme::drawRoundedPanel (g, inspector.toFloat(), c.surface, c.line.withAlpha (0.5f), 12.0f, 1.0f);
        g.setColour (c.text); g.setFont (juce::Font (15.0f, juce::Font::bold));
        g.drawText (("PAD " + juce::String (selectedPadIndex + 1) + "   " + selectedPadName).toUpperCase(),
                    inspector.reduced (16).removeFromTop (34), juce::Justification::centredLeft);
        g.setColour (c.textMuted); g.setFont (juce::Font (11.0f));
        g.drawText (takeCount > 0 ? juce::String (selectedTake + 1) + " of " + juce::String (takeCount) : "No hit",
                    inspector.reduced (16).withTrimmedTop (190).removeFromTop (24), juce::Justification::centred);
    }
}

void AurenMainView::resized()
{
    rack.setBounds (getLocalBounds()); rack.toBack();
    auto inner = innerBounds();
    auto header = inner.removeFromTop (58);
    previousKit.setBounds (header.removeFromLeft (420).removeFromRight (38));
    nextKit.setBounds (header.removeFromLeft (350).removeFromRight (38));
    save.setBounds (header.removeFromLeft (120).reduced (4));
    settings.setBounds (header.removeFromRight (90).reduced (4));
    masterOut.setBounds (header.removeFromRight (280).reduced (6));

    auto bottom = inner.removeFromBottom (86).reduced (12, 12);
    editMidi.setBounds (bottom.removeFromLeft (82).reduced (3));
    play.setBounds (bottom.removeFromLeft (66).reduced (3)); stop.setBounds (bottom.removeFromLeft (66).reduced (3));
    bpm.setBounds (bottom.removeFromLeft (115).reduced (4)); pattern.setBounds (bottom.removeFromLeft (150).reduced (4));
    barCount.setBounds (bottom.removeFromLeft (115).reduced (4)); dragMidi.setBounds (bottom.removeFromRight (190).reduced (4));

    if (midiMode)
    {
        midiEditor.setBounds (inner.reduced (8, 6));
        return;
    }

    auto sourceArea = inner.removeFromTop (225).reduced (8, 5);
    auto controls = sourceArea.removeFromRight (400);
    sensitivity.setBounds (controls.removeFromTop (48).reduced (5));
    auto actions = controls.removeFromTop (50);
    ai.setBounds (actions.removeFromLeft (95).reduced (5)); reclone.setBounds (actions.removeFromLeft (135).reduced (5));
    browse.setBounds (actions.removeFromLeft (135).reduced (5));
    source.setBounds (sourceArea);

    auto main = inner.reduced (8, 6);
    auto left = main.removeFromLeft (main.getWidth() * 25 / 100).reduced (0, 2);
    auto right = main.removeFromRight (main.getWidth() * 45 / 100).reduced (0, 2);
    auto centre = main.reduced (8, 2);
    assistant.setBounds (left.removeFromTop (left.getHeight() - 105));
    importAudio.setBounds (left.removeFromTop (48).removeFromLeft (left.getWidth() / 2).reduced (3));
    pads.setBounds (centre);

    auto inspector = right.reduced (16, 12);
    auto top = inspector.removeFromTop (36);
    padType.setBounds (top.removeFromRight (145).reduced (3)); solo.setBounds (top.removeFromRight (42).reduced (3));
    mute.setBounds (top.removeFromRight (42).reduced (3));
    padWaveform.setBounds (inspector.removeFromTop (130));
    auto takeRow = inspector.removeFromTop (40);
    previousTake.setBounds (takeRow.removeFromLeft (45).reduced (3)); nextTake.setBounds (takeRow.removeFromRight (45).reduced (3));
    load.setBounds (takeRow.removeFromRight (120).reduced (3));
    auto knobArea = inspector.removeFromTop (190);
    const auto knobWidth = knobArea.getWidth() / 4;
    const auto knobHeight = knobArea.getHeight() / 2;
    for (int i = 0; i < 8; ++i)
        knobs[static_cast<size_t> (i)]->setBounds (knobArea.getX() + (i % 4) * knobWidth,
                                                  knobArea.getY() + (i / 4) * knobHeight,
                                                  knobWidth, knobHeight);
    auto chokeRow = inspector.removeFromTop (42);
    choke.setBounds (chokeRow.removeFromRight (chokeRow.getWidth() * 70 / 100).reduced (3));
    output.setBounds (inspector.removeFromTop (42).reduced (3));
}
} // namespace auren::ui
