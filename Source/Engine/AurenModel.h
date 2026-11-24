#pragma once

#include "AurenTypes.h"
#include "Pattern.h"

#include <juce_data_structures/juce_data_structures.h>

#include <array>
#include <atomic>

namespace auren
{
class PadSlot
{
public:
    PadSlot();

    [[nodiscard]] std::shared_ptr<const PadSample> getSample() const noexcept;
    void setSample (std::shared_ptr<const PadSample> newSample) noexcept;
    void clearSample() noexcept;

    [[nodiscard]] PadParameters getParameters() const noexcept;
    void setParameters (PadParameters newParameters) noexcept;

private:
    std::shared_ptr<const PadSample> sample;
    std::shared_ptr<const PadParameters> parameters;
};

class AurenModel
{
public:
    AurenModel();

    [[nodiscard]] PadSlot& pad (int bank, int padIndex) noexcept;
    [[nodiscard]] const PadSlot& pad (int bank, int padIndex) const noexcept;
    [[nodiscard]] PadSlot& padFlat (int flatIndex) noexcept;
    [[nodiscard]] const PadSlot& padFlat (int flatIndex) const noexcept;

    [[nodiscard]] int getSelectedBank() const noexcept { return selectedBank.load(); }
    void setSelectedBank (int bank) noexcept;

    [[nodiscard]] bool anySoloed() const noexcept;
    [[nodiscard]] DrumType drumTypeForPad (int flatIndex) const noexcept;

    PatternLibrary& getPatterns() noexcept { return patterns; }
    const PatternLibrary& getPatterns() const noexcept { return patterns; }

    // Embedded audio makes session recall self-contained, at the cost of a larger tree.
    [[nodiscard]] juce::ValueTree toValueTree (bool embedAudio = true) const;
    void restoreFromValueTree (const juce::ValueTree& tree);

private:
    std::array<PadSlot, kTotalPads> pads;
    std::atomic<int> selectedBank { 0 };
    PatternLibrary patterns;
};
} // namespace auren
