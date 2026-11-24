#pragma once

#include "AurenTypes.h"

#include <juce_data_structures/juce_data_structures.h>

#include <functional>
#include <vector>

namespace auren
{
class AurenModel;
class SamplerEngine;

enum class GridResolution : int
{
    eighth,
    eighthTriplet,
    sixteenth,
    sixteenthTriplet,
    thirtySecond,
    thirtySecondTriplet,
    sixtyFourth,
    sixtyFourthTriplet
};

enum class VariationScope : int
{
    all,
    kick,
    snare,
    hats
};

struct PatternNote
{
    int flatPad = 0;
    double beat = 0.0;
    double lengthBeats = 0.25;
    float velocity = 1.0f;

    [[nodiscard]] bool operator== (const PatternNote&) const noexcept = default;
};

class Pattern
{
public:
    explicit Pattern (juce::String patternName = "Pattern 1");

    [[nodiscard]] const juce::String& getName() const noexcept { return name; }
    void setName (juce::String newName) { name = std::move (newName); }

    [[nodiscard]] int getBars() const noexcept { return bars; }
    void setBars (int newBars);

    [[nodiscard]] double getBpm() const noexcept { return bpm; }
    void setBpm (double newBpm) noexcept;

    [[nodiscard]] GridResolution getResolution() const noexcept { return resolution; }
    void setResolution (GridResolution newResolution) noexcept { resolution = newResolution; }

    [[nodiscard]] int getStepsPerBar() const noexcept;
    [[nodiscard]] int getTotalSteps() const noexcept { return getStepsPerBar() * bars; }
    [[nodiscard]] double getStepLengthBeats() const noexcept;
    [[nodiscard]] double getLengthBeats() const noexcept { return static_cast<double> (bars) * 4.0; }

    [[nodiscard]] const std::vector<PatternNote>& getNotes() const noexcept { return notes; }
    void setNotes (std::vector<PatternNote> replacement);
    void clear() noexcept { notes.clear(); }
    void addOrReplaceStep (int flatPad, int step, float velocity, double lengthInSteps = 1.0);
    void removeStep (int flatPad, int step);
    void removeNote (size_t index);
    void setNoteVelocity (size_t index, float velocity);
    [[nodiscard]] int findNoteAtStep (int flatPad, int step) const noexcept;

    // Produces a deterministic, editable variation. The source remains unchanged.
    [[nodiscard]] Pattern createVariation (VariationScope scope,
                                           float amount,
                                           const AurenModel& model,
                                           std::uint32_t seed) const;

    [[nodiscard]] juce::ValueTree toValueTree() const;
    static Pattern fromValueTree (const juce::ValueTree& tree);

private:
    void normaliseNotes();

    juce::String name;
    int bars = 2;
    double bpm = 120.0;
    GridResolution resolution = GridResolution::sixteenth;
    std::vector<PatternNote> notes;
};

class PatternLibrary
{
public:
    PatternLibrary();

    [[nodiscard]] int size() const noexcept { return static_cast<int> (patterns.size()); }
    [[nodiscard]] int getCurrentIndex() const noexcept { return currentIndex; }
    void setCurrentIndex (int index) noexcept;

    Pattern& current() noexcept;
    const Pattern& current() const noexcept;
    Pattern& add (Pattern pattern);
    void remove (int index);

    [[nodiscard]] const std::vector<Pattern>& all() const noexcept { return patterns; }
    [[nodiscard]] juce::ValueTree toValueTree() const;
    void restoreFromValueTree (const juce::ValueTree& tree);

private:
    std::vector<Pattern> patterns;
    int currentIndex = 0;
};

class PatternPlayer
{
public:
    using Trigger = std::function<void (int flatPad, float velocity)>;

    void prepare (double newSampleRate) noexcept;
    void reset() noexcept;
    void setPlaying (bool shouldPlay) noexcept { playing = shouldPlay; }
    [[nodiscard]] bool isPlaying() const noexcept { return playing; }
    [[nodiscard]] double getPlayheadBeat() const noexcept { return playheadBeat; }

    // Called once per audio block. Triggers all notes crossed by the block and loops.
    void process (int numSamples, const Pattern& pattern, const Trigger& trigger);

private:
    double sampleRate = 44100.0;
    double playheadBeat = 0.0;
    bool playing = false;
};
} // namespace auren
