#include "AurenControls.h"

namespace auren::ui
{
namespace
{
const Palette& fallbackPalette()
{
    static Theme fallback;
    return fallback.colours();
}

const Palette& coloursFor (Theme* theme)
{
    return theme != nullptr ? theme->colours() : fallbackPalette();
}
}

AurenButton::AurenButton (juce::String text) : juce::Button (text)
{
    setButtonText (std::move (text));
    setClickingTogglesState (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void AurenButton::setTheme (Theme* newTheme) { theme = newTheme; repaint(); }
void AurenButton::setAccent (bool value) { accent = value; repaint(); }
void AurenButton::setCompact (bool value) { compact = value; repaint(); }

void AurenButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto& c = coloursFor (theme);
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const bool active = getToggleState();
    auto fill = (accent || active) ? c.forest : c.surfaceRaised;
    auto textColour = (accent || active) ? juce::Colours::white : c.text;

    if (! isEnabled())
    {
        fill = c.surfaceSunken;
        textColour = c.textMuted.withAlpha (0.45f);
    }
    else if (down)
    {
        fill = fill.darker (0.22f);
    }
    else if (highlighted)
    {
        fill = fill.brighter (0.08f);
    }

    g.setColour (c.shadow.withAlpha (0.24f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), compact ? 7.0f : 9.0f);
    Theme::drawRoundedPanel (g, bounds, fill,
                             (accent || active ? c.forestBright : c.line).withAlpha (0.7f),
                             compact ? 7.0f : 9.0f, 1.0f);
    g.setColour (textColour);
    g.setFont (juce::Font (juce::jlimit (11.0f, 17.0f, bounds.getHeight() * 0.38f), juce::Font::bold));
    g.drawFittedText (getButtonText(), bounds.toNearestInt().reduced (6, 1),
                      juce::Justification::centred, 1);
}

AurenKnob::AurenKnob (juce::String label, juce::NormalisableRange<double> range, double initialValue)
    : name (std::move (label)), normalisableRange (std::move (range)),
      value (initialValue), defaultValue (initialValue)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void AurenKnob::setTheme (Theme* newTheme) { theme = newTheme; repaint(); }
void AurenKnob::setAccent (bool valueToUse) { accent = valueToUse; repaint(); }

void AurenKnob::setValue (double newValue, juce::NotificationType notification)
{
    newValue = normalisableRange.snapToLegalValue (juce::jlimit (normalisableRange.start,
                                                                 normalisableRange.end,
                                                                 newValue));
    if (juce::approximatelyEqual (newValue, value))
        return;
    value = newValue;
    repaint();
    if (notification != juce::dontSendNotification && onValueChange)
        onValueChange (value);
}

void AurenKnob::paint (juce::Graphics& g)
{
    const auto& c = coloursFor (theme);
    auto area = getLocalBounds().toFloat();
    auto labelArea = area.removeFromBottom (juce::jmin (20.0f, area.getHeight() * 0.28f));
    const auto diameter = juce::jmin (area.getWidth(), area.getHeight()) - 6.0f;
    auto knob = juce::Rectangle<float> (diameter, diameter).withCentre (area.getCentre());
    const auto proportion = static_cast<float> (normalisableRange.convertTo0to1 (value));
    constexpr float start = juce::MathConstants<float>::pi * 1.24f;
    constexpr float sweep = juce::MathConstants<float>::pi * 1.52f;
    const auto angle = start + sweep * proportion;

    g.setColour (c.shadow.withAlpha (0.4f));
    g.fillEllipse (knob.translated (0.0f, 2.0f));
    juce::ColourGradient face (c.surfaceRaised.brighter (0.1f), knob.getX(), knob.getY(),
                               c.surfaceSunken.darker (0.12f), knob.getRight(), knob.getBottom(), false);
    g.setGradientFill (face);
    g.fillEllipse (knob);
    g.setColour (c.line);
    g.drawEllipse (knob.reduced (0.5f), 1.2f);

    juce::Path arc;
    arc.addCentredArc (knob.getCentreX(), knob.getCentreY(), knob.getWidth() * 0.57f,
                       knob.getHeight() * 0.57f, 0.0f, start, start + sweep, true);
    g.setColour (c.line.withAlpha (0.42f));
    g.strokePath (arc, juce::PathStrokeType (2.4f));
    if (accent || proportion > 0.03f)
    {
        arc.clear();
        arc.addCentredArc (knob.getCentreX(), knob.getCentreY(), knob.getWidth() * 0.57f,
                           knob.getHeight() * 0.57f, 0.0f, start, angle, true);
        g.setColour (accent ? c.brass : c.forestBright);
        g.strokePath (arc, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    const auto centre = knob.getCentre();
    const auto pointerLength = knob.getWidth() * 0.34f;
    g.setColour (c.text);
    g.drawLine (centre.x, centre.y,
                centre.x + std::sin (angle) * pointerLength,
                centre.y - std::cos (angle) * pointerLength, 2.0f);

    g.setColour (c.text);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawFittedText (name.toUpperCase(), labelArea.toNearestInt(), juce::Justification::centred, 1);
}

void AurenKnob::mouseDown (const juce::MouseEvent&) { valueAtMouseDown = value; }

void AurenKnob::mouseDrag (const juce::MouseEvent& event)
{
    const auto delta = -static_cast<double> (event.getDistanceFromDragStartY()) / 160.0;
    auto proportion = normalisableRange.convertTo0to1 (valueAtMouseDown) + delta;
    setValue (normalisableRange.convertFrom0to1 (juce::jlimit (0.0, 1.0, proportion)));
}

void AurenKnob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    auto proportion = normalisableRange.convertTo0to1 (value) + static_cast<double> (wheel.deltaY) * 0.08;
    setValue (normalisableRange.convertFrom0to1 (juce::jlimit (0.0, 1.0, proportion)));
}

void AurenKnob::mouseDoubleClick (const juce::MouseEvent&) { setValue (defaultValue); }

AurenSlider::AurenSlider (juce::String label, double initialValue)
    : name (std::move (label)), value (juce::jlimit (0.0, 1.0, initialValue))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void AurenSlider::setTheme (Theme* newTheme) { theme = newTheme; repaint(); }

void AurenSlider::setValue (double newValue, juce::NotificationType notification)
{
    newValue = juce::jlimit (0.0, 1.0, newValue);
    if (juce::approximatelyEqual (newValue, value))
        return;
    value = newValue;
    repaint();
    if (notification != juce::dontSendNotification && onValueChange)
        onValueChange (value);
}

void AurenSlider::paint (juce::Graphics& g)
{
    const auto& c = coloursFor (theme);
    auto area = getLocalBounds().toFloat();
    if (showLabel)
    {
        auto label = area.removeFromLeft (juce::jmin (96.0f, area.getWidth() * 0.34f));
        g.setColour (c.text);
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawFittedText (name, label.toNearestInt(), juce::Justification::centredLeft, 1);
    }
    area = area.reduced (8.0f, area.getHeight() * 0.34f);
    const auto track = juce::Rectangle<float> (area.getX(), area.getCentreY() - 2.5f, area.getWidth(), 5.0f);
    g.setColour (c.line.withAlpha (0.42f));
    g.fillRoundedRectangle (track, 2.5f);
    g.setColour (c.forest);
    g.fillRoundedRectangle (track.withWidth (track.getWidth() * static_cast<float> (value)), 2.5f);
    const auto x = track.getX() + track.getWidth() * static_cast<float> (value);
    g.setColour (c.shadow.withAlpha (0.42f));
    g.fillEllipse (x - 8.0f, track.getCentreY() - 6.0f, 16.0f, 16.0f);
    g.setColour (c.brass);
    g.fillEllipse (x - 7.0f, track.getCentreY() - 8.0f, 14.0f, 14.0f);
    g.setColour (c.text.withAlpha (0.55f));
    g.drawEllipse (x - 7.0f, track.getCentreY() - 8.0f, 14.0f, 14.0f, 0.8f);
}

void AurenSlider::mouseDown (const juce::MouseEvent& event) { updateFromMouse (event.position); }
void AurenSlider::mouseDrag (const juce::MouseEvent& event) { updateFromMouse (event.position); }

void AurenSlider::updateFromMouse (juce::Point<float> position)
{
    auto area = getLocalBounds().toFloat();
    if (showLabel)
        area.removeFromLeft (juce::jmin (96.0f, area.getWidth() * 0.34f));
    area = area.reduced (8.0f, 0.0f);
    setValue ((position.x - area.getX()) / area.getWidth());
}

SegmentedControl::SegmentedControl (juce::StringArray items) : labels (std::move (items))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SegmentedControl::setTheme (Theme* newTheme) { theme = newTheme; repaint(); }
void SegmentedControl::setItems (juce::StringArray newItems) { labels = std::move (newItems); selected = 0; repaint(); }

void SegmentedControl::setSelectedIndex (int index, juce::NotificationType notification)
{
    const auto maxIndex = juce::jmax (0, labels.size() - 1);
    index = juce::jlimit (0, maxIndex, index);
    if (index == selected)
        return;
    selected = index;
    repaint();
    if (notification != juce::dontSendNotification && onSelectionChange)
        onSelectionChange (selected);
}

void SegmentedControl::paint (juce::Graphics& g)
{
    const auto& c = coloursFor (theme);
    if (labels.isEmpty())
        return;
    auto area = getLocalBounds().toFloat().reduced (0.5f);
    Theme::drawRoundedPanel (g, area, c.surfaceRaised, c.line.withAlpha (0.65f), 8.0f, 1.0f);
    const auto segmentWidth = area.getWidth() / static_cast<float> (labels.size());
    for (int i = 0; i < labels.size(); ++i)
    {
        auto segment = area.withX (area.getX() + segmentWidth * static_cast<float> (i)).withWidth (segmentWidth);
        if (i == selected)
        {
            auto active = accentOverride.isTransparent() ? c.forest : accentOverride;
            g.setColour (active);
            g.fillRoundedRectangle (segment.reduced (1.0f), 7.0f);
        }
        if (i > 0)
        {
            g.setColour (c.line.withAlpha (0.42f));
            g.drawVerticalLine (juce::roundToInt (segment.getX()), segment.getY() + 5.0f, segment.getBottom() - 5.0f);
        }
        g.setColour (i == selected ? juce::Colours::white : c.text);
        g.setFont (juce::Font (juce::jmin (15.0f, area.getHeight() * 0.42f), juce::Font::bold));
        g.drawFittedText (labels[i], segment.toNearestInt().reduced (4, 1), juce::Justification::centred, 1);
    }
}

void SegmentedControl::mouseDown (const juce::MouseEvent& event)
{
    if (labels.isEmpty())
        return;
    setSelectedIndex (juce::jlimit (0, labels.size() - 1,
                                    static_cast<int> (event.position.x / static_cast<float> (getWidth()) * labels.size())));
}

AssistantCard::AssistantCard()
{
    prompt.setMultiLine (false);
    prompt.setReturnKeyStartsNewLine (false);
    prompt.setTextToShowWhenEmpty ("Type a message...", juce::Colours::grey);
    prompt.setBorder (juce::BorderSize<int> (0));
    prompt.setIndents (8, 3);
    addAndMakeVisible (prompt);
    addAndMakeVisible (send);
    send.setAccent (true);
    send.onClick = [this]
    {
        const auto text = prompt.getText().trim();
        if (text.isNotEmpty() && onSubmit)
            onSubmit (text);
        if (text.isNotEmpty())
            prompt.clear();
    };
    prompt.onReturnKey = [this] { send.triggerClick(); };
}

void AssistantCard::setTheme (Theme* newTheme)
{
    theme = newTheme;
    send.setTheme (theme);
    const auto& c = coloursFor (theme);
    prompt.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    prompt.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    prompt.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    prompt.setColour (juce::TextEditor::textColourId, c.text);
    prompt.setColour (juce::TextEditor::highlightColourId, c.forest.withAlpha (0.35f));
    repaint();
}

void AssistantCard::setMessage (juce::String newMessage) { message = std::move (newMessage); repaint(); }
void AssistantCard::setPrompt (juce::String text) { prompt.setText (std::move (text), false); }
juce::String AssistantCard::getPrompt() const { return prompt.getText(); }

void AssistantCard::paint (juce::Graphics& g)
{
    const auto& c = coloursFor (theme);
    auto area = getLocalBounds().toFloat();
    Theme::drawRoundedPanel (g, area.reduced (1.0f), c.surfaceRaised, c.line.withAlpha (0.58f), 12.0f, 1.0f);
    auto content = area.reduced (16.0f, 12.0f);
    auto heading = content.removeFromTop (28.0f);
    g.setColour (c.brass);
    juce::Path sparkle;
    auto s = heading.removeFromLeft (26.0f).reduced (4.0f);
    sparkle.startNewSubPath (s.getCentreX(), s.getY());
    sparkle.lineTo (s.getCentreX() + 3.0f, s.getCentreY() - 3.0f);
    sparkle.lineTo (s.getRight(), s.getCentreY());
    sparkle.lineTo (s.getCentreX() + 3.0f, s.getCentreY() + 3.0f);
    sparkle.lineTo (s.getCentreX(), s.getBottom());
    sparkle.lineTo (s.getCentreX() - 3.0f, s.getCentreY() + 3.0f);
    sparkle.lineTo (s.getX(), s.getCentreY());
    sparkle.lineTo (s.getCentreX() - 3.0f, s.getCentreY() - 3.0f);
    sparkle.closeSubPath();
    g.strokePath (sparkle, juce::PathStrokeType (1.7f));
    g.setColour (c.text);
    g.setFont (juce::Font (15.0f, juce::Font::bold));
    g.drawFittedText ("AI ASSISTANT", heading.toNearestInt(), juce::Justification::centredLeft, 1);

    if (message.isNotEmpty())
    {
        auto messageArea = content;
        messageArea.removeFromBottom (40.0f);
        g.setColour (c.surfaceSunken.withAlpha (0.75f));
        g.fillRoundedRectangle (messageArea, 8.0f);
        g.setColour (c.text);
        g.setFont (juce::Font (12.0f));
        g.drawFittedText (message, messageArea.toNearestInt().reduced (10, 6),
                          juce::Justification::centredLeft, 4, 0.85f);
    }

    auto field = getLocalBounds().toFloat().reduced (16.0f, 12.0f).removeFromBottom (34.0f);
    field.removeFromRight (42.0f);
    g.setColour (c.surface);
    g.fillRoundedRectangle (field, 8.0f);
    g.setColour (c.line.withAlpha (0.6f));
    g.drawRoundedRectangle (field, 8.0f, 1.0f);
}

void AssistantCard::resized()
{
    auto area = getLocalBounds().reduced (16, 12).removeFromBottom (34);
    send.setBounds (area.removeFromRight (36));
    area.removeFromRight (6);
    prompt.setBounds (area);
}

RackFrame::RackFrame (Theme* initialTheme) : theme (initialTheme) { setInterceptsMouseClicks (false, false); }
void RackFrame::setTheme (Theme* newTheme) { theme = newTheme; repaint(); }

void RackFrame::paint (juce::Graphics& g)
{
    const auto& c = coloursFor (theme);
    auto bounds = getLocalBounds().toFloat();
    g.fillAll (c.rackEdge);
    g.setColour (c.rack);
    g.fillRoundedRectangle (bounds.reduced (2.0f), 17.0f);

    auto face = bounds.withTrimmedLeft (Metrics::rackEar).withTrimmedRight (Metrics::rackEar).reduced (4.0f);
    juce::ColourGradient metal (c.surfaceRaised, face.getX(), face.getY(), c.surface, face.getX(), face.getBottom(), false);
    g.setGradientFill (metal);
    g.fillRoundedRectangle (face, 15.0f);
    g.setColour (c.line.withAlpha (0.65f));
    g.drawRoundedRectangle (face.reduced (0.5f), 15.0f, 1.0f);

    const std::array<juce::Point<float>, 4> screws {
        juce::Point<float> (20.0f, 28.0f), juce::Point<float> (bounds.getRight() - 20.0f, 28.0f),
        juce::Point<float> (20.0f, bounds.getBottom() - 28.0f),
        juce::Point<float> (bounds.getRight() - 20.0f, bounds.getBottom() - 28.0f)
    };
    for (const auto point : screws)
        Theme::drawScrew (g, point, 7.0f, c.brass, c.rackEdge);

    g.setColour (c.rackEdge.withAlpha (0.9f));
    g.fillRoundedRectangle (7.0f, 92.0f, 26.0f, 10.0f, 5.0f);
    g.fillRoundedRectangle (bounds.getRight() - 33.0f, 92.0f, 26.0f, 10.0f, 5.0f);
    g.fillRoundedRectangle (7.0f, bounds.getBottom() - 102.0f, 26.0f, 10.0f, 5.0f);
    g.fillRoundedRectangle (bounds.getRight() - 33.0f, bounds.getBottom() - 102.0f, 26.0f, 10.0f, 5.0f);
}
} // namespace auren::ui
