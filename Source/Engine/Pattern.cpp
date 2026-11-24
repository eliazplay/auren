#include "Pattern.h"
#include "AurenModel.h"

#include <algorithm>
#include <cmath>

namespace auren
{
Pattern::Pattern (juce::String patternName) : name (std::move (patternName)) {}

void Pattern::setBars (int newBars) { bars = juce::jlimit (1, 8, newBars); normaliseNotes(); }
void Pattern::setBpm (double newBpm) noexcept { bpm = juce::jlimit (20.0, 320.0, newBpm); }

int Pattern::getStepsPerBar() const noexcept
{
    switch (resolution)
    {
        case GridResolution::eighth:              return 8;
        case GridResolution::eighthTriplet:       return 12;
        case GridResolution::sixteenth:           return 16;
        case GridResolution::sixteenthTriplet:    return 24;
        case GridResolution::thirtySecond:        return 32;
        case GridResolution::thirtySecondTriplet: return 48;
        case GridResolution::sixtyFourth:         return 64;
        case GridResolution::sixtyFourthTriplet:  return 96;
    }
    return 16;
}

double Pattern::getStepLengthBeats() const noexcept { return 4.0 / static_cast<double> (getStepsPerBar()); }

void Pattern::setNotes (std::vector<PatternNote> replacement) { notes = std::move (replacement); normaliseNotes(); }

void Pattern::addOrReplaceStep (int flatPad, int step, float velocity, double lengthInSteps)
{
    flatPad = juce::jlimit (0, kTotalPads - 1, flatPad);
    step = juce::jlimit (0, getTotalSteps() - 1, step);
    const auto beat = step * getStepLengthBeats();
    const auto existing = findNoteAtStep (flatPad, step);
    PatternNote note { flatPad, beat, juce::jmax (0.001, lengthInSteps * getStepLengthBeats()), juce::jlimit (0.0f, 1.0f, velocity) };
    if (existing >= 0) notes[static_cast<size_t> (existing)] = note;
    else notes.push_back (note);
    normaliseNotes();
}

void Pattern::removeStep (int flatPad, int step)
{
    if (const auto index = findNoteAtStep (flatPad, step); index >= 0)
        notes.erase (notes.begin() + index);
}

void Pattern::removeNote (size_t index) { if (index < notes.size()) notes.erase (notes.begin() + static_cast<std::ptrdiff_t> (index)); }
void Pattern::setNoteVelocity (size_t index, float velocity) { if (index < notes.size()) notes[index].velocity = juce::jlimit (0.0f, 1.0f, velocity); }

int Pattern::findNoteAtStep (int flatPad, int step) const noexcept
{
    const auto beat = step * getStepLengthBeats();
    const auto tolerance = getStepLengthBeats() * 0.25;
    for (size_t i = 0; i < notes.size(); ++i)
        if (notes[i].flatPad == flatPad && std::abs (notes[i].beat - beat) <= tolerance)
            return static_cast<int> (i);
    return -1;
}

Pattern Pattern::createVariation (VariationScope scope, float amount, const AurenModel& model, std::uint32_t seed) const
{
    Pattern result (*this);
    amount = juce::jlimit (0.0f, 1.0f, amount);
    juce::Random random (static_cast<juce::int64> (seed));
    const auto step = getStepLengthBeats();
    const auto matches = [&] (DrumType type)
    {
        if (scope == VariationScope::all) return true;
        if (scope == VariationScope::kick) return type == DrumType::kick;
        if (scope == VariationScope::snare) return type == DrumType::snare || type == DrumType::clap;
        return type == DrumType::hat || type == DrumType::openHat;
    };

    const auto source = notes;
    for (const auto& note : source)
    {
        const auto type = model.drumTypeForPad (note.flatPad);
        if (! matches (type)) continue;
        if (type == DrumType::kick && random.nextFloat() < amount * 0.38f)
            result.notes.push_back ({ note.flatPad, std::fmod (note.beat + step, getLengthBeats()), step, note.velocity * 0.82f });
        else if ((type == DrumType::snare || type == DrumType::clap) && random.nextFloat() < amount * 0.42f)
            result.notes.push_back ({ note.flatPad, juce::jmax (0.0, note.beat - step * 0.5), step * 0.35, juce::jlimit (0.15f, 0.55f, note.velocity * 0.45f) });
        else if ((type == DrumType::hat || type == DrumType::openHat) && random.nextFloat() < amount * 0.65f)
            result.notes.push_back ({ note.flatPad, std::fmod (note.beat + step * 0.5, getLengthBeats()), step * 0.45, note.velocity * (0.55f + 0.35f * random.nextFloat()) });
    }
    result.normaliseNotes();
    return result;
}

void Pattern::normaliseNotes()
{
    const auto length = getLengthBeats();
    for (auto& note : notes)
    {
        note.flatPad = juce::jlimit (0, kTotalPads - 1, note.flatPad);
        note.beat = juce::jlimit (0.0, std::nextafter (length, 0.0), note.beat);
        note.lengthBeats = juce::jlimit (0.001, length, note.lengthBeats);
        note.velocity = juce::jlimit (0.0f, 1.0f, note.velocity);
    }
    std::stable_sort (notes.begin(), notes.end(), [] (const auto& a, const auto& b)
    {
        return a.beat == b.beat ? a.flatPad < b.flatPad : a.beat < b.beat;
    });
}

juce::ValueTree Pattern::toValueTree() const
{
    juce::ValueTree tree ("PATTERN");
    tree.setProperty ("name", name, nullptr); tree.setProperty ("bars", bars, nullptr);
    tree.setProperty ("bpm", bpm, nullptr); tree.setProperty ("resolution", static_cast<int> (resolution), nullptr);
    for (const auto& note : notes)
    {
        juce::ValueTree n ("NOTE"); n.setProperty ("pad", note.flatPad, nullptr); n.setProperty ("beat", note.beat, nullptr);
        n.setProperty ("length", note.lengthBeats, nullptr); n.setProperty ("velocity", note.velocity, nullptr); tree.addChild (n, -1, nullptr);
    }
    return tree;
}

Pattern Pattern::fromValueTree (const juce::ValueTree& tree)
{
    Pattern result (tree.getProperty ("name", "Pattern").toString());
    result.bars = juce::jlimit (1, 8, static_cast<int> (tree.getProperty ("bars", 2)));
    result.bpm = juce::jlimit (20.0, 320.0, static_cast<double> (tree.getProperty ("bpm", 120.0)));
    result.resolution = static_cast<GridResolution> (static_cast<int> (tree.getProperty ("resolution", static_cast<int> (GridResolution::sixteenth))));
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        const auto n = tree.getChild (i); if (! n.hasType ("NOTE")) continue;
        result.notes.push_back ({ static_cast<int> (n.getProperty ("pad", 0)),
                                  static_cast<double> (n.getProperty ("beat", 0.0)),
                                  static_cast<double> (n.getProperty ("length", 0.25)),
                                  static_cast<float> (n.getProperty ("velocity", 1.0f)) });
    }
    result.normaliseNotes(); return result;
}

PatternLibrary::PatternLibrary() { patterns.emplace_back ("Pattern 1"); }
void PatternLibrary::setCurrentIndex (int index) noexcept { currentIndex = juce::jlimit (0, juce::jmax (0, size() - 1), index); }
Pattern& PatternLibrary::current() noexcept { return patterns[static_cast<size_t> (currentIndex)]; }
const Pattern& PatternLibrary::current() const noexcept { return patterns[static_cast<size_t> (currentIndex)]; }
Pattern& PatternLibrary::add (Pattern pattern) { patterns.push_back (std::move (pattern)); currentIndex = size() - 1; return current(); }
void PatternLibrary::remove (int index) { if (patterns.size() <= 1) return; patterns.erase (patterns.begin() + juce::jlimit (0, size() - 1, index)); setCurrentIndex (currentIndex); }
juce::ValueTree PatternLibrary::toValueTree() const { juce::ValueTree t ("PATTERNS"); t.setProperty ("current", currentIndex, nullptr); for (const auto& p : patterns) t.addChild (p.toValueTree(), -1, nullptr); return t; }
void PatternLibrary::restoreFromValueTree (const juce::ValueTree& tree) { if (! tree.hasType ("PATTERNS")) return; patterns.clear(); for (int i=0;i<tree.getNumChildren();++i) if (tree.getChild(i).hasType("PATTERN")) patterns.push_back(Pattern::fromValueTree(tree.getChild(i))); if(patterns.empty()) patterns.emplace_back("Pattern 1"); setCurrentIndex(static_cast<int> (tree.getProperty("current",0))); }

void PatternPlayer::prepare (double newSampleRate) noexcept { sampleRate = juce::jmax (1.0, newSampleRate); reset(); }
void PatternPlayer::reset() noexcept { playheadBeat = 0.0; }
void PatternPlayer::process (int numSamples, const Pattern& pattern, const Trigger& trigger)
{
    if (! playing || numSamples <= 0 || ! trigger) return;
    const auto length = pattern.getLengthBeats();
    const auto advance = static_cast<double> (numSamples) * pattern.getBpm() / (60.0 * sampleRate);
    const auto end = playheadBeat + advance;
    for (const auto& note : pattern.getNotes())
    {
        bool crossed = end < length ? (note.beat >= playheadBeat && note.beat < end)
                                   : (note.beat >= playheadBeat || note.beat < std::fmod (end, length));
        if (crossed) trigger (note.flatPad, note.velocity);
    }
    playheadBeat = std::fmod (end, length);
}
} // namespace auren
