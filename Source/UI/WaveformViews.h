#pragma once

#include "AurenControls.h"

namespace auren::ui
{
struct HitMarker
{
    float position = 0.0f;
    juce::String family;
};

struct FamilyCount
{
    juce::String family;
    int count = 0;
};

class SourceWaveformView final : public juce::Component, public ThemedComponent
{
public:
    void setTheme (Theme*) override;
    void setAudio (std::vector<float> monoSamples);
    void setMarkers (std::vector<HitMarker>);
    void setFamilyCounts (std::vector<FamilyCount>);
    void setMetadata (juce::String file, juce::String duration, int hitCount);
    void setVisibleRange (juce::Range<float> normalisedRange);

    void paint (juce::Graphics&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    Theme* theme = nullptr;
    std::vector<float> samples;
    std::vector<HitMarker> markers;
    std::vector<FamilyCount> counts;
    juce::String fileName { "Drop audio to begin" };
    juce::String duration;
    int totalHits = 0;
    juce::Range<float> visibleRange { 0.0f, 1.0f };
    juce::CriticalSection dataLock;
};

class PadWaveformView final : public juce::Component, public ThemedComponent
{
public:
    void setTheme (Theme*) override;
    void setAudio (std::vector<float> monoSamples, juce::String family);
    void clear();
    void setTrim (float start, float end);
    [[nodiscard]] juce::Range<float> getTrim() const noexcept { return trim; }

    std::function<void(float, float)> onTrimChanged;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    Theme* theme = nullptr;
    std::vector<float> samples;
    juce::String family { "Sample" };
    juce::Range<float> trim { 0.0f, 1.0f };
    bool draggingStart = false;
};

void drawWaveform (juce::Graphics&, juce::Rectangle<float>, const std::vector<float>&,
                   juce::Range<float>, juce::Colour, bool filled = true);
} // namespace auren::ui
