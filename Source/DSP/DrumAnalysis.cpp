#include "DrumAnalysis.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace auren::dsp
{
namespace
{
constexpr float epsilon = 1.0e-12f;

int nextPowerOfTwo (int value)
{
    int result = 1;
    while (result < value)
        result <<= 1;
    return result;
}

float median (std::vector<float> values)
{
    if (values.empty())
        return 0.0f;

    const auto middle = values.begin() + static_cast<std::ptrdiff_t> (values.size() / 2);
    std::nth_element (values.begin(), middle, values.end());
    float result = *middle;

    if ((values.size() & 1U) == 0U)
    {
        const auto lower = std::max_element (values.begin(), middle);
        result = 0.5f * (result + *lower);
    }

    return result;
}

std::vector<float> makeMono (const juce::AudioBuffer<float>& source)
{
    std::vector<float> mono (static_cast<std::size_t> (source.getNumSamples()), 0.0f);
    const auto channels = juce::jmax (1, source.getNumChannels());

    for (int channel = 0; channel < source.getNumChannels(); ++channel)
    {
        const auto* input = source.getReadPointer (channel);
        for (int sample = 0; sample < source.getNumSamples(); ++sample)
            mono[static_cast<std::size_t> (sample)] += input[sample] / static_cast<float> (channels);
    }

    return mono;
}

std::int64_t refineOnset (const std::vector<float>& mono,
                          std::int64_t coarsePosition,
                          int radius)
{
    const auto begin = juce::jlimit<std::int64_t> (1, static_cast<std::int64_t> (mono.size()) - 1,
                                                   coarsePosition - radius);
    const auto end = juce::jlimit<std::int64_t> (begin + 1, static_cast<std::int64_t> (mono.size()),
                                                 coarsePosition + radius);

    float bestScore = -1.0f;
    std::int64_t bestPosition = begin;

    // A short forward energy difference locates the leading edge more reliably
    // than the largest sample, particularly for kicks with a slow first cycle.
    constexpr int lookAhead = 12;
    for (auto position = begin; position < end; ++position)
    {
        float score = std::abs (mono[static_cast<std::size_t> (position)]
                                - mono[static_cast<std::size_t> (position - 1)]);

        for (int offset = 1; offset < lookAhead && position + offset < end; ++offset)
            score += 0.12f * std::abs (mono[static_cast<std::size_t> (position + offset)]);

        if (score > bestScore)
        {
            bestScore = score;
            bestPosition = position;
        }
    }

    // Walk back to the nearest low-energy crossing. It avoids trimming the tiny
    // initial click that defines the character of a drum hit.
    const float crossingLimit = bestScore * 0.015f;
    for (int back = 0; back < 32 && bestPosition > begin; ++back)
    {
        if (std::abs (mono[static_cast<std::size_t> (bestPosition - 1)]) <= crossingLimit)
            break;
        --bestPosition;
    }

    return bestPosition;
}

std::array<float, featureCount> measureFeatures (const std::vector<float>& mono,
                                                 std::int64_t onset,
                                                 double sampleRate,
                                                 float strength)
{
    std::array<float, featureCount> features {};
    const auto desiredWindow = static_cast<int> (sampleRate * 0.095);
    const auto fftSize = juce::jlimit (512, 8192, nextPowerOfTwo (desiredWindow));
    const auto order = static_cast<int> (std::log2 (static_cast<double> (fftSize)));
    juce::dsp::FFT fft (order);

    std::vector<float> fftData (static_cast<std::size_t> (2 * fftSize), 0.0f);
    int crossings = 0;
    float previous = 0.0f;
    double earlyEnergy = 0.0;

    for (int index = 0; index < fftSize; ++index)
    {
        const auto position = onset + index;
        const float value = position >= 0 && position < static_cast<std::int64_t> (mono.size())
                                ? mono[static_cast<std::size_t> (position)]
                                : 0.0f;
        const float window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                                    * static_cast<float> (index)
                                                    / static_cast<float> (fftSize - 1));
        fftData[static_cast<std::size_t> (index)] = value * window;
        earlyEnergy += static_cast<double> (value) * value;
        if (index > 0 && ((value >= 0.0f) != (previous >= 0.0f)))
            ++crossings;
        previous = value;
    }

    fft.performFrequencyOnlyForwardTransform (fftData.data());

    double low = 0.0, mid = 0.0, high = 0.0;
    double weightedFrequency = 0.0, magnitudeSum = 0.0;
    double logMagnitudeSum = 0.0;
    int usefulBins = 0;

    const int nyquistBin = fftSize / 2;
    for (int bin = 1; bin <= nyquistBin; ++bin)
    {
        const double frequency = static_cast<double> (bin) * sampleRate / fftSize;
        const double magnitude = juce::jmax (static_cast<double> (fftData[static_cast<std::size_t> (bin)]),
                                             static_cast<double> (epsilon));
        const double energy = magnitude * magnitude;

        if (frequency < 180.0)
            low += energy;
        else if (frequency < 2600.0)
            mid += energy;
        else
            high += energy;

        if (frequency <= 16000.0)
        {
            weightedFrequency += frequency * magnitude;
            magnitudeSum += magnitude;
            logMagnitudeSum += std::log (magnitude);
            ++usefulBins;
        }
    }

    const double total = low + mid + high + epsilon;
    features[lowBandRatio] = static_cast<float> (low / total);
    features[midBandRatio] = static_cast<float> (mid / total);
    features[highBandRatio] = static_cast<float> (high / total);
    features[spectralCentroid] = static_cast<float> (juce::jlimit (0.0, 1.0,
        weightedFrequency / (magnitudeSum + epsilon) / 12000.0));
    features[zeroCrossingRate] = static_cast<float> (crossings) / static_cast<float> (fftSize);

    const auto tailBegin = onset + static_cast<std::int64_t> (sampleRate * 0.12);
    const auto tailEnd = onset + static_cast<std::int64_t> (sampleRate * 0.28);
    double tailEnergy = 0.0;
    int tailSamples = 0;
    for (auto position = tailBegin;
         position < tailEnd && position < static_cast<std::int64_t> (mono.size()); ++position)
    {
        if (position >= 0)
        {
            const auto value = mono[static_cast<std::size_t> (position)];
            tailEnergy += static_cast<double> (value) * value;
            ++tailSamples;
        }
    }

    const auto earlyRms = std::sqrt (earlyEnergy / juce::jmax (1, fftSize));
    const auto tailRms = std::sqrt (tailEnergy / juce::jmax (1, tailSamples));
    features[decayRatio] = static_cast<float> (juce::jlimit (0.0, 1.0, tailRms / (earlyRms + epsilon)));

    const double arithmeticMean = magnitudeSum / juce::jmax (1, usefulBins);
    const double geometricMean = std::exp (logMagnitudeSum / juce::jmax (1, usefulBins));
    features[spectralFlatness] = static_cast<float> (juce::jlimit (0.0, 1.0,
        geometricMean / (arithmeticMean + epsilon)));
    features[transientStrength] = juce::jlimit (0.0f, 1.0f, strength);
    return features;
}

float featureDistance (const HitDescriptor& hit,
                       const std::array<float, featureCount>& centre)
{
    // Family ratios and centroid are more useful for timbre identity than the
    // hit level. The weights keep ghost notes in the same sound group.
    constexpr std::array<float, featureCount> weights { 1.3f, 1.0f, 1.1f, 1.3f,
                                                         0.7f, 0.8f, 0.7f, 0.2f };
    float sum = 0.0f;
    float weightSum = 0.0f;
    for (std::size_t index = 0; index < featureCount; ++index)
    {
        const auto difference = hit.features[index] - centre[index];
        sum += weights[index] * difference * difference;
        weightSum += weights[index];
    }
    return std::sqrt (sum / weightSum);
}
} // namespace

std::vector<HitDescriptor> DrumAnalyzer::analyze (const juce::AudioBuffer<float>& source,
                                                  double sampleRate,
                                                  const AnalysisOptions& requestedOptions)
{
    std::vector<HitDescriptor> hits;
    if (source.getNumChannels() == 0 || source.getNumSamples() < 64 || sampleRate <= 0.0)
        return hits;

    auto options = requestedOptions;
    options.analysisFftSize = juce::jlimit (256, 4096, nextPowerOfTwo (options.analysisFftSize));
    options.hopSize = juce::jlimit (32, options.analysisFftSize / 2, options.hopSize);
    options.sensitivity = juce::jlimit (0.0f, 1.0f, options.sensitivity);

    const auto mono = makeMono (source);
    const auto order = static_cast<int> (std::log2 (static_cast<double> (options.analysisFftSize)));
    juce::dsp::FFT fft (order);
    std::vector<float> previousMagnitude (static_cast<std::size_t> (options.analysisFftSize / 2 + 1), 0.0f);
    std::vector<float> fftData (static_cast<std::size_t> (2 * options.analysisFftSize), 0.0f);
    std::vector<float> flux;
    std::vector<float> frameEnergy;

    for (int frameStart = 0; frameStart + options.analysisFftSize <= static_cast<int> (mono.size());
         frameStart += options.hopSize)
    {
        double energy = 0.0;
        for (int index = 0; index < options.analysisFftSize; ++index)
        {
            const float window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                                        * static_cast<float> (index)
                                                        / static_cast<float> (options.analysisFftSize - 1));
            const auto value = mono[static_cast<std::size_t> (frameStart + index)];
            fftData[static_cast<std::size_t> (index)] = value * window;
            energy += static_cast<double> (value) * value;
        }
        std::fill (fftData.begin() + options.analysisFftSize, fftData.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        double positiveDifference = 0.0;
        double magnitudeTotal = 0.0;
        for (int bin = 1; bin <= options.analysisFftSize / 2; ++bin)
        {
            const auto magnitude = fftData[static_cast<std::size_t> (bin)];
            positiveDifference += juce::jmax (0.0f, magnitude - previousMagnitude[static_cast<std::size_t> (bin)]);
            magnitudeTotal += magnitude;
            previousMagnitude[static_cast<std::size_t> (bin)] = magnitude;
        }
        flux.push_back (static_cast<float> (positiveDifference / (magnitudeTotal + epsilon)));
        frameEnergy.push_back (static_cast<float> (std::sqrt (energy / options.analysisFftSize)));
    }

    if (flux.size() < 5)
        return hits;

    const auto fluxMedian = median (flux);
    std::vector<float> deviations;
    deviations.reserve (flux.size());
    for (const auto value : flux)
        deviations.push_back (std::abs (value - fluxMedian));
    const auto mad = median (std::move (deviations)) + epsilon;
    const float thresholdMultiplier = 4.7f - 3.1f * options.sensitivity;
    const auto globalThreshold = fluxMedian + thresholdMultiplier * mad;
    const auto minimumGap = static_cast<std::int64_t> (sampleRate * options.minimumGapMs / 1000.0);
    std::int64_t lastOnset = -minimumGap;

    const auto maximumFlux = *std::max_element (flux.begin(), flux.end());
    for (std::size_t frame = 2; frame + 2 < flux.size(); ++frame)
    {
        std::vector<float> localWindow;
        const auto localBegin = frame > 12 ? frame - 12 : 0;
        const auto localEnd = juce::jmin (flux.size(), frame + 13);
        localWindow.insert (localWindow.end(), flux.begin() + static_cast<std::ptrdiff_t> (localBegin),
                            flux.begin() + static_cast<std::ptrdiff_t> (localEnd));
        const auto localMedian = median (localWindow);
        for (auto& value : localWindow)
            value = std::abs (value - localMedian);
        const auto localMad = median (std::move (localWindow)) + epsilon;
        const auto threshold = juce::jmax (globalThreshold,
                                           localMedian + thresholdMultiplier * localMad);

        const bool localPeak = flux[frame] > flux[frame - 1]
                            && flux[frame] >= flux[frame + 1]
                            && flux[frame] > threshold;
        if (! localPeak)
            continue;

        const auto coarse = static_cast<std::int64_t> (frame * static_cast<std::size_t> (options.hopSize));
        const auto onset = refineOnset (mono, coarse, options.hopSize);
        if (onset - lastOnset < minimumGap)
        {
            if (! hits.empty() && flux[frame] > hits.back().strength * maximumFlux)
            {
                hits.back().samplePosition = onset;
                hits.back().strength = juce::jlimit (0.0f, 1.0f, flux[frame] / (maximumFlux + epsilon));
                hits.back().features = measureFeatures (mono, onset, sampleRate, hits.back().strength);
            }
            continue;
        }

        HitDescriptor hit;
        hit.samplePosition = onset;
        hit.strength = juce::jlimit (0.0f, 1.0f, flux[frame] / (maximumFlux + epsilon));
        hit.features = measureFeatures (mono, onset, sampleRate, hit.strength);
        hits.push_back (hit);
        lastOnset = onset;
        if (static_cast<int> (hits.size()) >= options.maxHits)
            break;
    }

    classifyAndGroup (hits, options);
    return hits;
}

DrumFamily DrumAnalyzer::classify (const std::array<float, featureCount>& f)
{
    const auto low = f[lowBandRatio];
    const auto mid = f[midBandRatio];
    const auto high = f[highBandRatio];
    const auto centroid = f[spectralCentroid];
    const auto zcr = f[zeroCrossingRate];
    const auto decay = f[decayRatio];
    const auto flatness = f[spectralFlatness];

    if (low > 0.50f && centroid < 0.27f)
        return DrumFamily::kick;
    if (low > 0.27f && mid > 0.34f && centroid < 0.42f && zcr < 0.20f)
        return DrumFamily::tom;
    if (high > 0.62f && decay < 0.19f)
        return DrumFamily::closedHat;
    if (high > 0.50f && decay > 0.48f)
        return decay > 0.72f ? DrumFamily::cymbal : DrumFamily::openHat;
    if (mid > 0.30f && high > 0.28f)
    {
        if (high > 0.48f && decay < 0.22f && flatness > 0.18f)
            return DrumFamily::clap;
        return DrumFamily::snare;
    }
    if (high > 0.45f && zcr > 0.12f)
        return DrumFamily::closedHat;
    if (low > 0.20f || mid > 0.34f)
        return DrumFamily::percussion;
    return DrumFamily::unknown;
}

void DrumAnalyzer::classifyAndGroup (std::vector<HitDescriptor>& hits,
                                     const AnalysisOptions& options)
{
    for (auto& hit : hits)
    {
        hit.family = classify (hit.features);
        hit.groupId = -1;
    }
    groupSimilarHits (hits, options);
}

void DrumAnalyzer::groupSimilarHits (std::vector<HitDescriptor>& hits,
                                     const AnalysisOptions& options)
{
    struct Group
    {
        DrumFamily family = DrumFamily::unknown;
        std::array<float, featureCount> centre {};
        int count = 0;
        int id = -1;
    };

    std::vector<Group> groups;
    int nextId = 0;
    for (auto& hit : hits)
    {
        Group* closest = nullptr;
        float closestDistance = std::numeric_limits<float>::max();
        int familyGroupCount = 0;

        for (auto& group : groups)
        {
            if (group.family != hit.family)
                continue;
            ++familyGroupCount;
            const auto distance = featureDistance (hit, group.centre);
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closest = &group;
            }
        }

        if (closest == nullptr
            || (closestDistance > options.groupingDistance
                && familyGroupCount < options.maximumGroupsPerFamily))
        {
            groups.push_back ({ hit.family, hit.features, 1, nextId++ });
            hit.groupId = groups.back().id;
            continue;
        }

        hit.groupId = closest->id;
        ++closest->count;
        const auto blend = 1.0f / static_cast<float> (closest->count);
        for (std::size_t index = 0; index < featureCount; ++index)
            closest->centre[index] += blend * (hit.features[index] - closest->centre[index]);
    }
}
} // namespace auren::dsp
