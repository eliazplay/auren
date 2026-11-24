#include "SamplerEngine.h"

#include <algorithm>
#include <cmath>

namespace auren
{
void SamplerEngine::OnePole::set (double sampleRate, float hp, float lp) noexcept
{
    lowCoefficient = std::exp (-juce::MathConstants<float>::twoPi * lp / (float) sampleRate);
    highCoefficient = std::exp (-juce::MathConstants<float>::twoPi * hp / (float) sampleRate);
}

float SamplerEngine::OnePole::process (float input) noexcept
{
    // LP after a simple DC/high-pass tracker. The high-pass component protects
    // low-end from offsets and subsonic artifacts in extracted hits.
    highTrackingState = highCoefficient * highTrackingState + (1.0f - highCoefficient) * input;
    auto high = input - highTrackingState;
    lowState = lowCoefficient * lowState + (1.0f - lowCoefficient) * high;
    return lowState;
}

void SamplerEngine::prepare (double sampleRate, int, int outputChannels)
{
    outputSampleRate = sampleRate;
    numOutputChannels = outputChannels;
    reset();
}

void SamplerEngine::reset() noexcept
{
    for (auto& voice : voices) { voice = {}; }
}

SamplerEngine::Voice& SamplerEngine::findVoiceToUse() noexcept
{
    for (auto& voice : voices) if (! voice.active) return voice;
    return *std::min_element (voices.begin(), voices.end(), [] (const Voice& a, const Voice& b)
    { return a.ageSamples > b.ageSamples; });
}

void SamplerEngine::chokeGroup (int group) noexcept
{
    if (group == 0) return;
    for (auto& voice : voices)
        if (voice.active && voice.choke == group) voice.release ((int) (outputSampleRate * 0.008));
}

void SamplerEngine::triggerPad (int bank, int padIndex, float velocity)
{
    triggerFlatPad (flatPadIndex (bank, padIndex), velocity);
}

void SamplerEngine::triggerFlatPad (int flat, float velocity)
{
    flat = juce::jlimit (0, kTotalPads - 1, flat);
    auto sample = model.padFlat (flat).getSample();
    if (sample == nullptr || ! sample->hasOriginal()) return;
    auto params = model.padFlat (flat).getParameters();
    if (params.muted || (model.anySoloed() && ! params.soloed)) return;
    chokeGroup (params.chokeGroup);
    auto& voice = findVoiceToUse();
    voice = {};
    voice.active = true; voice.flatPad = flat; voice.choke = params.chokeGroup;
    voice.sample = std::move (sample); voice.parameters = params;
    voice.velocity = juce::jlimit (0.0f, 1.0f, velocity);
    voice.increment = voice.sample->sourceSampleRate / outputSampleRate
                    * std::pow (2.0, params.tuneSemitones / 12.0);
    for (auto& filter : voice.filters) filter.set (outputSampleRate, params.highPassHz, params.lowPassHz);
}

float SamplerEngine::readSample (const juce::AudioBuffer<float>& buffer, int channel, double pos) const noexcept
{
    const int i = (int) pos; const float frac = (float) (pos - i);
    if (i < 0 || i >= buffer.getNumSamples()) return 0.0f;
    const auto a = buffer.getSample (juce::jmin (channel, buffer.getNumChannels()-1), i);
    const auto b = buffer.getSample (juce::jmin (channel, buffer.getNumChannels()-1), juce::jmin (i+1, buffer.getNumSamples()-1));
    return a + (b-a)*frac;
}

void SamplerEngine::renderRange (juce::AudioBuffer<float>& output, const FamilyOutputs&, int start, int count)
{
    for (auto& v : voices)
    {
        if (!v.active || v.sample == nullptr) continue;
        const auto& original = v.sample->original;
        const auto& isolated = v.sample->hasIsolated() ? v.sample->isolated : original;
        const auto gain = juce::Decibels::decibelsToGain (v.parameters.gainDb) * v.velocity;
        for (int i=0;i<count;++i)
        {
            if (v.sourcePosition >= original.getNumSamples()) { v.active=false; break; }
            float env = 1.0f;
            const int attack = (int)(v.parameters.attackMs * outputSampleRate / 1000.0);
            if (attack > 0) env *= juce::jlimit (0.0f, 1.0f, (float)v.ageSamples / attack);
            if (v.releaseSamplesLeft > 0) { env *= (float)v.releaseSamplesLeft / v.releaseSamplesTotal; --v.releaseSamplesLeft; if (v.releaseSamplesLeft <= 0) {v.active=false; break;} }
            const auto clean = v.parameters.clean;
            for (int ch=0;ch<juce::jmin (2, output.getNumChannels());++ch)
            {
                float s = (readSample(original,ch,v.sourcePosition)*(1.0f-clean)+readSample(isolated,ch,v.sourcePosition)*clean);
                s = v.filters[(size_t)ch].process(s)*gain*env;
                const float pan = v.parameters.pan;
                const float panGain = ch == 0 ? std::sqrt(0.5f*(1.0f-pan)) : std::sqrt(0.5f*(1.0f+pan));
                output.addSample(ch,start+i,s*panGain);
            }
            v.sourcePosition += v.increment; ++v.ageSamples;
        }
    }
}

void SamplerEngine::processBlock (juce::AudioBuffer<float>& output, const FamilyOutputs& family, const juce::MidiBuffer& midi, int bank)
{
    for (const auto meta : midi)
        if (meta.getMessage().isNoteOn())
        {
            const int local = meta.getMessage().getNoteNumber() - kFirstPadMidiNote;
            if (local >= 0 && local < kPadsPerBank) triggerPad(bank, local, meta.getMessage().getFloatVelocity());
        }
    renderRange(output, family, 0, output.getNumSamples());
}
} // namespace auren
