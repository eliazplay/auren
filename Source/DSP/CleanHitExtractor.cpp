#include "CleanHitExtractor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace auren::dsp
{
namespace
{
constexpr float epsilon = 1.0e-9f;

struct AlignedHit
{
    std::int64_t position = 0;
    float polarity = 1.0f;
    float correlation = 1.0f;
};

struct Biquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

    void process (juce::AudioBuffer<float>& audio) const
    {
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        {
            double z1 = 0.0, z2 = 0.0;
            auto* samples = audio.getWritePointer (channel);
            for (int index = 0; index < audio.getNumSamples(); ++index)
            {
                const double input = samples[index];
                const double output = b0 * input + z1;
                z1 = b1 * input - a1 * output + z2;
                z2 = b2 * input - a2 * output;
                samples[index] = static_cast<float> (output);
            }
        }
    }
};

Biquad makeHighPass (double sampleRate, double frequency, double q = 0.70710678118)
{
    const auto omega = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0 * q);
    const auto a0 = 1.0 + alpha;
    return { ((1.0 + cosine) * 0.5) / a0,
             -(1.0 + cosine) / a0,
             ((1.0 + cosine) * 0.5) / a0,
             (-2.0 * cosine) / a0,
             (1.0 - alpha) / a0 };
}

Biquad makePeakFilter (double sampleRate, double frequency, double q, double gainDb)
{
    const auto a = std::pow (10.0, gainDb / 40.0);
    const auto omega = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0 * q);
    const auto a0 = 1.0 + alpha / a;
    return { (1.0 + alpha * a) / a0,
             (-2.0 * cosine) / a0,
             (1.0 - alpha * a) / a0,
             (-2.0 * cosine) / a0,
             (1.0 - alpha / a) / a0 };
}

std::vector<float> makeMono (const juce::AudioBuffer<float>& source)
{
    std::vector<float> result (static_cast<std::size_t> (source.getNumSamples()), 0.0f);
    const auto channels = juce::jmax (1, source.getNumChannels());
    for (int channel = 0; channel < source.getNumChannels(); ++channel)
    {
        const auto* samples = source.getReadPointer (channel);
        for (int index = 0; index < source.getNumSamples(); ++index)
            result[static_cast<std::size_t> (index)] += samples[index] / static_cast<float> (channels);
    }
    return result;
}

float correlationAt (const std::vector<float>& mono,
                     std::int64_t reference,
                     std::int64_t candidate,
                     int length)
{
    double dot = 0.0, a2 = 0.0, b2 = 0.0;
    for (int index = 0; index < length; ++index)
    {
        const auto aPosition = reference + index;
        const auto bPosition = candidate + index;
        if (aPosition < 0 || bPosition < 0
            || aPosition >= static_cast<std::int64_t> (mono.size())
            || bPosition >= static_cast<std::int64_t> (mono.size()))
            break;
        const auto a = mono[static_cast<std::size_t> (aPosition)];
        const auto b = mono[static_cast<std::size_t> (bPosition)];
        dot += static_cast<double> (a) * b;
        a2 += static_cast<double> (a) * a;
        b2 += static_cast<double> (b) * b;
    }
    return static_cast<float> (dot / std::sqrt (a2 * b2 + epsilon));
}

std::vector<AlignedHit> alignHits (const std::vector<float>& mono,
                                   const std::vector<const HitDescriptor*>& selected,
                                   double sampleRate,
                                   const ExtractionOptions& options)
{
    std::vector<AlignedHit> aligned;
    if (selected.empty())
        return aligned;

    const auto reference = selected.front()->samplePosition;
    const auto search = juce::jmax (1, static_cast<int> (sampleRate * options.alignmentWindowMs / 1000.0));
    const auto compareLength = juce::jmax (32, static_cast<int> (sampleRate * options.correlationWindowMs / 1000.0));
    aligned.push_back ({ reference, 1.0f, 1.0f });

    for (std::size_t hitIndex = 1; hitIndex < selected.size(); ++hitIndex)
    {
        float best = 0.0f;
        int bestOffset = 0;
        for (int offset = -search; offset <= search; ++offset)
        {
            const auto value = correlationAt (mono, reference, selected[hitIndex]->samplePosition + offset,
                                              compareLength);
            if (std::abs (value) > std::abs (best))
            {
                best = value;
                bestOffset = offset;
            }
        }

        if (selected.size() <= 3 || std::abs (best) >= options.minimumCorrelation)
            aligned.push_back ({ selected[hitIndex]->samplePosition + bestOffset,
                                 best < 0.0f ? -1.0f : 1.0f,
                                 std::abs (best) });
    }
    return aligned;
}

float robustCentre (std::vector<float>& values, float trimFraction, float& deviation)
{
    std::sort (values.begin(), values.end());
    if (values.empty())
    {
        deviation = 0.0f;
        return 0.0f;
    }

    const auto median = values[values.size() / 2];
    std::vector<float> deviations;
    deviations.reserve (values.size());
    for (const auto value : values)
        deviations.push_back (std::abs (value - median));
    std::nth_element (deviations.begin(), deviations.begin() + static_cast<std::ptrdiff_t> (deviations.size() / 2),
                      deviations.end());
    deviation = deviations[deviations.size() / 2] * 1.4826f;

    if (values.size() < 4)
        return median;

    const auto trim = juce::jlimit<std::size_t> (0, (values.size() - 1) / 2,
        static_cast<std::size_t> (std::floor (values.size() * trimFraction)));
    const auto first = values.begin() + static_cast<std::ptrdiff_t> (trim);
    const auto last = values.end() - static_cast<std::ptrdiff_t> (trim);
    return std::accumulate (first, last, 0.0f) / static_cast<float> (std::distance (first, last));
}

float defaultLengthMs (DrumFamily family)
{
    switch (family)
    {
        case DrumFamily::kick:       return 850.0f;
        case DrumFamily::snare:      return 650.0f;
        case DrumFamily::clap:       return 420.0f;
        case DrumFamily::closedHat:  return 240.0f;
        case DrumFamily::openHat:    return 1350.0f;
        case DrumFamily::tom:        return 950.0f;
        case DrumFamily::percussion: return 620.0f;
        case DrumFamily::cymbal:     return 1800.0f;
        case DrumFamily::unknown:    return 800.0f;
    }
    return 800.0f;
}

float estimateFundamental (const juce::AudioBuffer<float>& audio, double sampleRate,
                           std::vector<float>* spectrumOut = nullptr)
{
    const int fftSize = 16384;
    juce::dsp::FFT fft (14);
    std::vector<float> data (static_cast<std::size_t> (2 * fftSize), 0.0f);
    const auto start = juce::jmin (audio.getNumSamples(), static_cast<int> (sampleRate * 0.018));
    for (int index = 0; index < fftSize && start + index < audio.getNumSamples(); ++index)
    {
        float mono = 0.0f;
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            mono += audio.getSample (channel, start + index) / audio.getNumChannels();
        const auto window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                                   * index / static_cast<float> (fftSize - 1));
        data[static_cast<std::size_t> (index)] = mono * window;
    }
    fft.performFrequencyOnlyForwardTransform (data.data());
    if (spectrumOut != nullptr)
        spectrumOut->assign (data.begin(), data.begin() + fftSize / 2 + 1);

    const auto firstBin = juce::jmax (1, static_cast<int> (35.0 * fftSize / sampleRate));
    const auto lastBin = juce::jmin (fftSize / 2, static_cast<int> (150.0 * fftSize / sampleRate));
    int bestBin = firstBin;
    for (int bin = firstBin + 1; bin <= lastBin; ++bin)
        if (data[static_cast<std::size_t> (bin)] > data[static_cast<std::size_t> (bestBin)])
            bestBin = bin;
    return static_cast<float> (bestBin * sampleRate / fftSize);
}

void removeDc (juce::AudioBuffer<float>& audio)
{
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
    {
        auto* samples = audio.getWritePointer (channel);
        const auto mean = std::accumulate (samples, samples + audio.getNumSamples(), 0.0) /
                          juce::jmax (1, audio.getNumSamples());
        for (int index = 0; index < audio.getNumSamples(); ++index)
            samples[index] -= static_cast<float> (mean);
    }
}

void suppressNarrowKickResonances (juce::AudioBuffer<float>& audio, double sampleRate,
                                   float fundamental)
{
    std::vector<float> spectrum;
    estimateFundamental (audio, sampleRate, &spectrum);
    constexpr int fftSize = 16384;
    struct Candidate { float prominence = 0.0f; float frequency = 0.0f; };
    std::vector<Candidate> candidates;
    const auto first = static_cast<int> (180.0 * fftSize / sampleRate);
    const auto last = juce::jmin (static_cast<int> (1400.0 * fftSize / sampleRate),
                                  static_cast<int> (spectrum.size()) - 8);

    for (int bin = juce::jmax (8, first); bin < last; ++bin)
    {
        const auto frequency = static_cast<float> (bin * sampleRate / fftSize);
        const auto harmonic = juce::jmax (1, static_cast<int> (std::round (frequency / fundamental)));
        if (std::abs (frequency - harmonic * fundamental) / frequency < 0.065f)
            continue; // Preserve the musical harmonics that give the kick its pitch.

        float neighbours = 0.0f;
        int count = 0;
        for (int offset = -8; offset <= 8; ++offset)
        {
            if (std::abs (offset) > 2)
            {
                neighbours += spectrum[static_cast<std::size_t> (bin + offset)];
                ++count;
            }
        }
        neighbours /= juce::jmax (1, count);
        const auto prominence = 20.0f * std::log10 ((spectrum[static_cast<std::size_t> (bin)] + epsilon)
                                                    / (neighbours + epsilon));
        if (prominence > 11.0f)
            candidates.push_back ({ prominence, frequency });
    }

    std::sort (candidates.begin(), candidates.end(), [] (const auto& a, const auto& b)
    {
        return a.prominence > b.prominence;
    });
    for (int index = 0; index < juce::jmin (2, static_cast<int> (candidates.size())); ++index)
    {
        const auto attenuation = juce::jlimit (-4.5f, -1.5f,
                                               -1.5f - 0.35f * (candidates[index].prominence - 11.0f));
        makePeakFilter (sampleRate, candidates[index].frequency, 6.0, attenuation).process (audio);
    }
}

void trimTailAndFade (juce::AudioBuffer<float>& audio, double sampleRate,
                      DrumFamily family, float fadeMs)
{
    const auto frame = juce::jmax (16, static_cast<int> (sampleRate * 0.010));
    float peakRms = 0.0f;
    std::vector<float> rms;
    for (int start = 0; start < audio.getNumSamples(); start += frame)
    {
        double energy = 0.0;
        int count = 0;
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int sample = start; sample < juce::jmin (audio.getNumSamples(), start + frame); ++sample)
            {
                const auto value = audio.getSample (channel, sample);
                energy += static_cast<double> (value) * value;
                ++count;
            }
        const auto value = static_cast<float> (std::sqrt (energy / juce::jmax (1, count)));
        rms.push_back (value);
        peakRms = juce::jmax (peakRms, value);
    }

    const auto minimumMs = family == DrumFamily::kick ? 260.0f
                         : family == DrumFamily::closedHat ? 70.0f : 120.0f;
    int lastActiveFrame = juce::jmin (static_cast<int> (rms.size()) - 1,
        static_cast<int> (minimumMs / 10.0f));
    const auto threshold = peakRms * juce::Decibels::decibelsToGain (-48.0f);
    for (int index = lastActiveFrame; index < static_cast<int> (rms.size()); ++index)
        if (rms[static_cast<std::size_t> (index)] > threshold)
            lastActiveFrame = index;

    const auto fadeSamples = juce::jmax (16, static_cast<int> (sampleRate * fadeMs / 1000.0));
    const auto newLength = juce::jlimit (fadeSamples, audio.getNumSamples(),
                                        (lastActiveFrame + 1) * frame + fadeSamples);
    audio.setSize (audio.getNumChannels(), newLength, true, true, true);
    const auto fadeStart = juce::jmax (0, newLength - fadeSamples);
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        audio.applyGainRamp (channel, fadeStart, newLength - fadeStart, 1.0f, 0.0f);
}

void normalizeWithHeadroom (juce::AudioBuffer<float>& audio, float targetDb)
{
    const auto peak = audio.getMagnitude (0, audio.getNumSamples());
    if (peak <= epsilon)
        return;
    const auto target = juce::Decibels::decibelsToGain (juce::jmin (-0.3f, targetDb));
    // Cap upward gain so a very quiet extraction does not turn residual noise
    // into the dominant part of the sample.
    audio.applyGain (juce::jmin (target / peak, juce::Decibels::decibelsToGain (12.0f)));
}

ExtractionResult extractSelected (const juce::AudioBuffer<float>& source,
                                  double sampleRate,
                                  std::vector<const HitDescriptor*> selected,
                                  DrumFamily family,
                                  const ExtractionOptions& options)
{
    ExtractionResult result;
    result.sampleRate = sampleRate;
    result.family = family;
    if (source.getNumChannels() == 0 || source.getNumSamples() == 0
        || sampleRate <= 0.0 || selected.empty())
    {
        result.status = "No usable hits";
        return result;
    }

    std::stable_sort (selected.begin(), selected.end(), [] (const auto* a, const auto* b)
    {
        return a->strength > b->strength;
    });
    if (static_cast<int> (selected.size()) > options.maximumOccurrences)
        selected.resize (static_cast<std::size_t> (options.maximumOccurrences));

    const auto mono = makeMono (source);
    const auto aligned = alignHits (mono, selected, sampleRate, options);
    if (aligned.empty())
    {
        result.status = "Hits could not be aligned";
        return result;
    }

    const auto lengthMs = juce::jmin (defaultLengthMs (family), options.maximumLengthMs);
    const auto length = juce::jmax (32, static_cast<int> (sampleRate * lengthMs / 1000.0));
    const auto channels = juce::jlimit (1, 2, source.getNumChannels());
    result.audio.setSize (channels, length);
    result.audio.clear();
    std::vector<float> values;
    values.reserve (aligned.size());

    const auto protectTransient = static_cast<int> (sampleRate * 0.025);
    for (int channel = 0; channel < channels; ++channel)
    {
        auto* output = result.audio.getWritePointer (channel);
        for (int sample = 0; sample < length; ++sample)
        {
            values.clear();
            for (const auto& hit : aligned)
            {
                const auto position = hit.position + sample;
                values.push_back (position >= 0 && position < source.getNumSamples()
                                      ? source.getSample (channel, static_cast<int> (position)) * hit.polarity
                                      : 0.0f);
            }
            float deviation = 0.0f;
            const auto centre = robustCentre (values, options.trimmedFraction, deviation);
            const auto coherence = std::abs (centre) / (std::abs (centre) + deviation + epsilon);
            const auto uncertainGain = juce::jlimit (0.28f, 1.0f, std::sqrt (coherence));
            const auto cleanGain = sample < protectTransient ? 1.0f
                : 1.0f + options.cleanAmount * (uncertainGain - 1.0f);
            output[sample] = centre * cleanGain;
        }
    }

    result.usedHitCount = static_cast<int> (aligned.size());
    result.confidence = std::accumulate (aligned.begin(), aligned.end(), 0.0f,
        [] (float sum, const auto& hit) { return sum + hit.correlation; }) / aligned.size();

    removeDc (result.audio);
    result.detectedFundamentalHz = family == DrumFamily::kick
        ? estimateFundamental (result.audio, sampleRate) : 0.0f;

    if (options.removeSubsonics)
    {
        const auto cutoff = family == DrumFamily::kick
            ? juce::jlimit (18.0, 30.0, static_cast<double> (result.detectedFundamentalHz) * 0.38)
            : 18.0;
        makeHighPass (sampleRate, cutoff).process (result.audio);
    }

    if (family == DrumFamily::kick && options.protectKickResonances
        && result.detectedFundamentalHz > 0.0f)
        suppressNarrowKickResonances (result.audio, sampleRate, result.detectedFundamentalHz);

    trimTailAndFade (result.audio, sampleRate, family, options.fadeOutMs);
    normalizeWithHeadroom (result.audio, options.targetPeakDb);
    result.status = result.usedHitCount > 1
        ? juce::String (result.usedHitCount) + " aligned hits"
        : "Single-hit cleanup";
    return result;
}
} // namespace

ExtractionResult CleanHitExtractor::extractGroup (const juce::AudioBuffer<float>& source,
                                                   double sampleRate,
                                                   const std::vector<HitDescriptor>& hits,
                                                   int groupId,
                                                   const ExtractionOptions& options)
{
    std::vector<const HitDescriptor*> selected;
    DrumFamily family = DrumFamily::unknown;
    for (const auto& hit : hits)
        if (hit.groupId == groupId)
        {
            selected.push_back (&hit);
            family = hit.family;
        }
    return extractSelected (source, sampleRate, std::move (selected), family, options);
}

ExtractionResult CleanHitExtractor::extractSingle (const juce::AudioBuffer<float>& source,
                                                    double sampleRate,
                                                    const HitDescriptor& hit,
                                                    const ExtractionOptions& options)
{
    return extractSelected (source, sampleRate, { &hit }, hit.family, options);
}
} // namespace auren::dsp
