#pragma once

#include <JuceHeader.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

namespace auren::ui
{
enum class Skin
{
    light,
    dark
};

struct Palette
{
    juce::Colour rack;
    juce::Colour rackEdge;
    juce::Colour surface;
    juce::Colour surfaceRaised;
    juce::Colour surfaceSunken;
    juce::Colour panelDark;
    juce::Colour panelDarkAlt;
    juce::Colour line;
    juce::Colour text;
    juce::Colour textMuted;
    juce::Colour forest;
    juce::Colour forestBright;
    juce::Colour brass;
    juce::Colour shadow;
    juce::Colour kick;
    juce::Colour snare;
    juce::Colour clap;
    juce::Colour hat;
    juce::Colour tom;
    juce::Colour perc;
    juce::Colour cymbal;
};

class Theme
{
public:
    explicit Theme (Skin initialSkin = Skin::light);

    void setSkin (Skin newSkin) noexcept;
    [[nodiscard]] Skin getSkin() const noexcept { return skin; }
    [[nodiscard]] const Palette& colours() const noexcept { return palette; }

    [[nodiscard]] juce::Colour familyColour (const juce::String& family) const;

    static void drawRoundedPanel (juce::Graphics&,
                                  juce::Rectangle<float>,
                                  juce::Colour fill,
                                  juce::Colour stroke,
                                  float radius = 12.0f,
                                  float strokeWidth = 1.0f);
    static void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius,
                           juce::Colour metal, juce::Colour slot);
    static void drawWaveMark (juce::Graphics&, juce::Rectangle<float>, juce::Colour);

private:
    void updatePalette();

    Skin skin;
    Palette palette;
};

struct Metrics
{
    static constexpr float designWidth = 1584.0f;
    static constexpr float designHeight = 991.0f;
    static constexpr float rackEar = 45.0f;
    static constexpr float frame = 18.0f;
    static constexpr float panelRadius = 14.0f;
};
} // namespace auren::ui
