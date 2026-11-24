#pragma once

#include "DrumTypes.h"

namespace auren::dsp
{
class DrumAnalyzer
{
public:
    [[nodiscard]] static std::vector<HitDescriptor> analyze (
        const juce::AudioBuffer<float>& source,
        double sampleRate,
        const AnalysisOptions& options = {});

    // Re-runs only the inexpensive family/group stage. This is useful after a
    // user changes a tag or the grouping sensitivity in the UI.
    static void classifyAndGroup (std::vector<HitDescriptor>& hits,
                                  const AnalysisOptions& options = {});

    [[nodiscard]] static DrumFamily classify (
        const std::array<float, featureCount>& features);

private:
    static void groupSimilarHits (std::vector<HitDescriptor>& hits,
                                  const AnalysisOptions& options);
};
} // namespace auren::dsp
