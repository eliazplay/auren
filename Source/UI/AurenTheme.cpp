#include "AurenTheme.h"

namespace auren::ui
{
Theme::Theme (Skin initialSkin) : skin (initialSkin)
{
    updatePalette();
}

void Theme::setSkin (Skin newSkin) noexcept
{
    skin = newSkin;
    updatePalette();
}

void Theme::updatePalette()
{
    if (skin == Skin::light)
    {
        palette = {
            juce::Colour::fromRGB (37, 73, 59),    // rack
            juce::Colour::fromRGB (18, 43, 34),    // rack edge
            juce::Colour::fromRGB (229, 234, 226), // surface
            juce::Colour::fromRGB (242, 244, 238), // raised
            juce::Colour::fromRGB (209, 217, 208), // sunken
            juce::Colour::fromRGB (14, 48, 37),    // dark panel
            juce::Colour::fromRGB (18, 57, 44),    // dark panel alt
            juce::Colour::fromRGB (126, 139, 127), // line
            juce::Colour::fromRGB (13, 44, 35),    // text
            juce::Colour::fromRGB (92, 108, 99),   // muted
            juce::Colour::fromRGB (9, 84, 55),     // forest
            juce::Colour::fromRGB (31, 139, 89),   // forest bright
            juce::Colour::fromRGB (191, 148, 43),  // brass
            juce::Colour (0x3508120e),              // shadow
            juce::Colour::fromRGB (247, 132, 48),  // kick
            juce::Colour::fromRGB (94, 224, 160),  // snare
            juce::Colour::fromRGB (232, 86, 145),  // clap
            juce::Colour::fromRGB (73, 203, 226),  // hat
            juce::Colour::fromRGB (166, 114, 228), // tom
            juce::Colour::fromRGB (236, 106, 153), // perc
            juce::Colour::fromRGB (170, 190, 247)  // cymbal
        };
    }
    else
    {
        palette = {
            juce::Colour::fromRGB (12, 22, 27),    // rack
            juce::Colour::fromRGB (4, 10, 13),     // rack edge
            juce::Colour::fromRGB (20, 29, 31),    // surface
            juce::Colour::fromRGB (29, 41, 42),    // raised
            juce::Colour::fromRGB (11, 18, 21),    // sunken
            juce::Colour::fromRGB (7, 19, 19),     // dark panel
            juce::Colour::fromRGB (10, 28, 25),    // dark panel alt
            juce::Colour::fromRGB (74, 92, 90),    // line
            juce::Colour::fromRGB (230, 236, 226), // text
            juce::Colour::fromRGB (145, 158, 151), // muted
            juce::Colour::fromRGB (22, 112, 78),   // forest
            juce::Colour::fromRGB (62, 210, 139),  // forest bright
            juce::Colour::fromRGB (218, 166, 54),  // brass
            juce::Colour (0x8a000000),              // shadow
            juce::Colour::fromRGB (255, 138, 53),
            juce::Colour::fromRGB (95, 229, 167),
            juce::Colour::fromRGB (241, 92, 154),
            juce::Colour::fromRGB (80, 211, 235),
            juce::Colour::fromRGB (176, 123, 238),
            juce::Colour::fromRGB (240, 112, 161),
            juce::Colour::fromRGB (178, 200, 250)
        };
    }
}

juce::Colour Theme::familyColour (const juce::String& family) const
{
    const auto name = family.trim().toLowerCase();
    if (name.contains ("kick"))   return palette.kick;
    if (name.contains ("snare"))  return palette.snare;
    if (name.contains ("clap"))   return palette.clap;
    if (name.contains ("hat"))    return palette.hat;
    if (name.contains ("tom"))    return palette.tom;
    if (name.contains ("perc"))   return palette.perc;
    if (name.contains ("cymbal")) return palette.cymbal;
    return palette.forestBright;
}

void Theme::drawRoundedPanel (juce::Graphics& g, juce::Rectangle<float> bounds,
                              juce::Colour fill, juce::Colour stroke,
                              float radius, float strokeWidth)
{
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, radius);
    g.setColour (stroke);
    g.drawRoundedRectangle (bounds.reduced (strokeWidth * 0.5f), radius, strokeWidth);
}

void Theme::drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius,
                       juce::Colour metal, juce::Colour slot)
{
    juce::ColourGradient gradient (metal.brighter (0.45f), centre.x - radius, centre.y - radius,
                                   metal.darker (0.45f), centre.x + radius, centre.y + radius, false);
    g.setGradientFill (gradient);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (slot.withAlpha (0.75f));
    g.drawLine (centre.x - radius * 0.5f, centre.y + radius * 0.2f,
                centre.x + radius * 0.5f, centre.y - radius * 0.2f, 1.4f);
}

void Theme::drawWaveMark (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour)
{
    const std::array<float, 7> heights { 0.22f, 0.50f, 0.82f, 1.0f, 0.72f, 0.44f, 0.20f };
    g.setColour (colour);
    const auto gap = area.getWidth() / 9.0f;
    for (size_t i = 0; i < heights.size(); ++i)
    {
        const auto x = area.getX() + gap * static_cast<float> (i + 1);
        const auto h = area.getHeight() * heights[i];
        g.drawLine (x, area.getCentreY() - h * 0.5f, x, area.getCentreY() + h * 0.5f,
                    juce::jmax (2.0f, gap * 0.34f));
    }
}
} // namespace auren::ui
