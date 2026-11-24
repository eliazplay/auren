#include "WaveformViews.h"

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

void drawWaveform (juce::Graphics& g, juce::Rectangle<float> bounds,
                   const std::vector<float>& samples, juce::Range<float> range,
                   juce::Colour colour, bool filled)
{
    if (samples.empty() || bounds.getWidth() < 2.0f)
    {
        g.setColour (colour.withAlpha (0.22f));
        g.drawHorizontalLine (juce::roundToInt (bounds.getCentreY()), bounds.getX(), bounds.getRight());
        return;
    }

    const auto first = juce::jlimit<size_t> (0, samples.size() - 1,
                                             static_cast<size_t> (range.getStart() * static_cast<float> (samples.size())));
    const auto last = juce::jlimit<size_t> (first + 1, samples.size(),
                                            static_cast<size_t> (range.getEnd() * static_cast<float> (samples.size())));
    const auto pixels = juce::jmax (2, juce::roundToInt (bounds.getWidth()));
    juce::Path upper, lower;
    for (int x = 0; x < pixels; ++x)
    {
        const auto a = first + static_cast<size_t> ((last - first) * static_cast<size_t> (x) / static_cast<size_t> (pixels));
        const auto b = juce::jmin (last, first + static_cast<size_t> ((last - first) * static_cast<size_t> (x + 1) / static_cast<size_t> (pixels)) + 1);
        float lo = 0.0f, hi = 0.0f;
        for (auto i = a; i < b; ++i)
        {
            lo = juce::jmin (lo, samples[i]);
            hi = juce::jmax (hi, samples[i]);
        }
        const auto px = bounds.getX() + static_cast<float> (x);
        const auto yHi = bounds.getCentreY() - hi * bounds.getHeight() * 0.46f;
        const auto yLo = bounds.getCentreY() - lo * bounds.getHeight() * 0.46f;
        if (x == 0) { upper.startNewSubPath (px, yHi); lower.startNewSubPath (px, yLo); }
        else { upper.lineTo (px, yHi); lower.lineTo (px, yLo); }
    }
    g.setColour (colour);
    if (filled)
    {
        juce::Path shape = upper;
        std::vector<juce::Point<float>> lowerPoints;
        for (int x = pixels - 1; x >= 0; --x)
        {
            const auto a = first + static_cast<size_t> ((last - first) * static_cast<size_t> (x) / static_cast<size_t> (pixels));
            const auto b = juce::jmin (last, first + static_cast<size_t> ((last - first) * static_cast<size_t> (x + 1) / static_cast<size_t> (pixels)) + 1);
            float lo = 0.0f;
            for (auto i = a; i < b; ++i) lo = juce::jmin (lo, samples[i]);
            shape.lineTo (bounds.getX() + static_cast<float> (x),
                          bounds.getCentreY() - lo * bounds.getHeight() * 0.46f);
        }
        shape.closeSubPath();
        g.fillPath (shape);
    }
    else
    {
        g.strokePath (upper, juce::PathStrokeType (1.2f));
        g.strokePath (lower, juce::PathStrokeType (1.2f));
    }
}

void SourceWaveformView::setTheme (Theme* newTheme) { theme = newTheme; repaint(); }
void SourceWaveformView::setAudio (std::vector<float> data) { const juce::ScopedLock lock (dataLock); samples = std::move (data); repaint(); }
void SourceWaveformView::setMarkers (std::vector<HitMarker> data) { const juce::ScopedLock lock (dataLock); markers = std::move (data); repaint(); }
void SourceWaveformView::setFamilyCounts (std::vector<FamilyCount> data) { counts = std::move (data); repaint(); }
void SourceWaveformView::setMetadata (juce::String file, juce::String time, int hits) { fileName = std::move (file); duration = std::move (time); totalHits = hits; repaint(); }
void SourceWaveformView::setVisibleRange (juce::Range<float> range) { visibleRange = range.constrainedWithin ({ 0.0f, 1.0f }); repaint(); }

void SourceWaveformView::paint (juce::Graphics& g)
{
    const juce::ScopedLock lock (dataLock);
    const auto& c = paletteFor (theme);
    auto area = getLocalBounds().toFloat();
    Theme::drawRoundedPanel (g, area.reduced (1.0f), c.panelDark, c.line.withAlpha (0.55f), 12.0f, 1.0f);
    auto content = area.reduced (14.0f, 10.0f);
    auto header = content.removeFromTop (24.0f);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.setColour (juce::Colours::white.withAlpha (0.62f));
    g.drawText ("SOURCE", header.removeFromLeft (70.0f), juce::Justification::centredLeft);
    g.setColour (juce::Colours::white);
    g.drawFittedText (fileName, header.removeFromLeft (220.0f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.drawText (duration, header.removeFromLeft (52.0f), juce::Justification::centredLeft);
    if (totalHits > 0) g.drawText (juce::String (totalHits) + " hits", header.removeFromLeft (75.0f), juce::Justification::centredLeft);

    auto legend = content.removeFromBottom (25.0f);
    auto wave = content.reduced (0.0f, 3.0f);
    g.setColour (c.panelDarkAlt);
    g.fillRoundedRectangle (wave, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawHorizontalLine (juce::roundToInt (wave.getCentreY()), wave.getX(), wave.getRight());
    drawWaveform (g, wave.reduced (7.0f), samples, visibleRange, juce::Colours::white.withAlpha (0.25f));
    for (const auto& marker : markers)
    {
        if (! visibleRange.contains (marker.position)) continue;
        const auto normal = (marker.position - visibleRange.getStart()) / visibleRange.getLength();
        const auto x = wave.getX() + wave.getWidth() * normal;
        const auto colour = theme != nullptr ? theme->familyColour (marker.family) : c.forestBright;
        g.setColour (colour.withAlpha (0.86f));
        g.drawLine (x, wave.getY() + 5.0f, x, wave.getBottom() - 5.0f, 1.15f);
        g.fillRoundedRectangle (x - 1.6f, wave.getY() + 4.0f, 3.2f, 6.0f, 1.2f);
    }

    float x = legend.getX();
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    for (const auto& item : counts)
    {
        const auto colour = theme != nullptr ? theme->familyColour (item.family) : c.forestBright;
        g.setColour (colour);
        g.fillEllipse (x, legend.getCentreY() - 3.5f, 7.0f, 7.0f);
        g.setColour (juce::Colours::white.withAlpha (0.88f));
        const auto label = item.family.toUpperCase() + "  " + juce::String (item.count);
        g.drawText (label, juce::Rectangle<float> (x + 11.0f, legend.getY(), 78.0f, legend.getHeight()), juce::Justification::centredLeft);
        x += 91.0f;
    }
}

void SourceWaveformView::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    const auto oldLength = visibleRange.getLength();
    const auto newLength = juce::jlimit (0.05f, 1.0f, oldLength * (wheel.deltaY > 0.0f ? 0.82f : 1.22f));
    const auto anchor = juce::jlimit (0.0f, 1.0f, event.position.x / static_cast<float> (getWidth()));
    const auto centre = visibleRange.getStart() + anchor * oldLength;
    setVisibleRange ({ centre - anchor * newLength, centre + (1.0f - anchor) * newLength });
}

void PadWaveformView::setTheme (Theme* newTheme) { theme = newTheme; repaint(); }
void PadWaveformView::setAudio (std::vector<float> data, juce::String newFamily) { samples = std::move (data); family = std::move (newFamily); trim = { 0.0f, 1.0f }; repaint(); }
void PadWaveformView::clear() { samples.clear(); repaint(); }
void PadWaveformView::setTrim (float start, float end) { trim = { juce::jlimit (0.0f, 1.0f, start), juce::jlimit (0.0f, 1.0f, end) }; repaint(); }

void PadWaveformView::paint (juce::Graphics& g)
{
    const auto& c = paletteFor (theme);
    auto area = getLocalBounds().toFloat();
    Theme::drawRoundedPanel (g, area.reduced (0.5f), c.panelDark, c.line.withAlpha (0.65f), 8.0f, 1.0f);
    if (samples.empty())
    {
        Theme::drawWaveMark (g, area.withSizeKeepingCentre (70.0f, 28.0f), c.textMuted);
        g.setColour (juce::Colours::white.withAlpha (0.58f));
        g.setFont (juce::Font (11.0f));
        g.drawText ("Empty pad", area.removeFromBottom (28.0f), juce::Justification::centred);
        return;
    }
    const auto colour = theme != nullptr ? theme->familyColour (family) : c.forestBright;
    drawWaveform (g, area.reduced (10.0f, 8.0f), samples, { 0.0f, 1.0f }, colour);
    const auto startX = area.getX() + trim.getStart() * area.getWidth();
    const auto endX = area.getX() + trim.getEnd() * area.getWidth();
    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.fillRect (juce::Rectangle<float> (area.getX(), area.getY(),
                                         juce::jmax (0.0f, startX - area.getX()), area.getHeight()));
    g.fillRect (juce::Rectangle<float> (endX, area.getY(),
                                         juce::jmax (0.0f, area.getRight() - endX), area.getHeight()));
    g.setColour (juce::Colours::white.withAlpha (0.78f));
    g.drawLine (startX, area.getY() + 3.0f, startX, area.getBottom() - 3.0f, 1.5f);
    g.drawLine (endX, area.getY() + 3.0f, endX, area.getBottom() - 3.0f, 1.5f);
}

void PadWaveformView::mouseDown (const juce::MouseEvent& event)
{
    const auto normal = event.position.x / static_cast<float> (getWidth());
    draggingStart = std::abs (normal - trim.getStart()) < std::abs (normal - trim.getEnd());
}

void PadWaveformView::mouseDrag (const juce::MouseEvent& event)
{
    const auto normal = juce::jlimit (0.0f, 1.0f, event.position.x / static_cast<float> (getWidth()));
    if (draggingStart) trim.setStart (juce::jmin (normal, trim.getEnd() - 0.01f));
    else trim.setEnd (juce::jmax (normal, trim.getStart() + 0.01f));
    repaint();
    if (onTrimChanged) onTrimChanged (trim.getStart(), trim.getEnd());
}
} // namespace auren::ui
