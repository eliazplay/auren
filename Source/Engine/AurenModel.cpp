#include "AurenModel.h"

#include <juce_core/juce_core.h>

namespace auren
{
namespace
{
juce::String encodeBuffer (const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return {};

    juce::MemoryOutputStream stream;
    stream.writeInt (buffer.getNumChannels());
    stream.writeInt (buffer.getNumSamples());
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            stream.writeFloat (buffer.getSample (channel, sample));

    return juce::Base64::toBase64 (stream.getData(), stream.getDataSize());
}

juce::AudioBuffer<float> decodeBuffer (const juce::String& encoded)
{
    juce::MemoryOutputStream decoded;
    if (encoded.isEmpty() || ! juce::Base64::convertFromBase64 (decoded, encoded))
        return {};

    juce::MemoryInputStream stream (decoded.getData(), decoded.getDataSize(), false);
    const auto channels = stream.readInt();
    const auto samples = stream.readInt();
    if (channels < 1 || channels > 8 || samples < 1 || samples > 30000000)
        return {};

    juce::AudioBuffer<float> result (channels, samples);
    for (int channel = 0; channel < channels; ++channel)
        for (int sample = 0; sample < samples; ++sample)
            result.setSample (channel, sample, stream.readFloat());
    return result;
}

void writeParameters (juce::ValueTree& tree, const PadParameters& p)
{
    tree.setProperty ("clean", p.clean, nullptr);
    tree.setProperty ("gainDb", p.gainDb, nullptr);
    tree.setProperty ("pan", p.pan, nullptr);
    tree.setProperty ("tune", p.tuneSemitones, nullptr);
    tree.setProperty ("attackMs", p.attackMs, nullptr);
    tree.setProperty ("fadeMs", p.fadeMs, nullptr);
    tree.setProperty ("highPassHz", p.highPassHz, nullptr);
    tree.setProperty ("lowPassHz", p.lowPassHz, nullptr);
    tree.setProperty ("chokeGroup", p.chokeGroup, nullptr);
    tree.setProperty ("muted", p.muted, nullptr);
    tree.setProperty ("soloed", p.soloed, nullptr);
    tree.setProperty ("sendToMain", p.sendToMain, nullptr);
    tree.setProperty ("outputFamily", static_cast<int> (p.outputFamily), nullptr);
}

PadParameters readParameters (const juce::ValueTree& tree)
{
    PadParameters p;
    p.clean = static_cast<float> (tree.getProperty ("clean", p.clean));
    p.gainDb = static_cast<float> (tree.getProperty ("gainDb", p.gainDb));
    p.pan = static_cast<float> (tree.getProperty ("pan", p.pan));
    p.tuneSemitones = static_cast<float> (tree.getProperty ("tune", p.tuneSemitones));
    p.attackMs = static_cast<float> (tree.getProperty ("attackMs", p.attackMs));
    p.fadeMs = static_cast<float> (tree.getProperty ("fadeMs", p.fadeMs));
    p.highPassHz = static_cast<float> (tree.getProperty ("highPassHz", p.highPassHz));
    p.lowPassHz = static_cast<float> (tree.getProperty ("lowPassHz", p.lowPassHz));
    p.chokeGroup = static_cast<int> (tree.getProperty ("chokeGroup", p.chokeGroup));
    p.muted = static_cast<bool> (tree.getProperty ("muted", p.muted));
    p.soloed = static_cast<bool> (tree.getProperty ("soloed", p.soloed));
    p.sendToMain = static_cast<bool> (tree.getProperty ("sendToMain", p.sendToMain));
    p.outputFamily = static_cast<OutputFamily> (static_cast<int> (tree.getProperty ("outputFamily", 0)));
    p.sanitise();
    return p;
}
} // namespace

PadSlot::PadSlot()
{
    std::atomic_store (&parameters, std::make_shared<const PadParameters>());
}

std::shared_ptr<const PadSample> PadSlot::getSample() const noexcept
{
    return std::atomic_load_explicit (&sample, std::memory_order_acquire);
}

void PadSlot::setSample (std::shared_ptr<const PadSample> newSample) noexcept
{
    std::atomic_store_explicit (&sample, std::move (newSample), std::memory_order_release);
}

void PadSlot::clearSample() noexcept { setSample ({}); }

PadParameters PadSlot::getParameters() const noexcept
{
    if (const auto p = std::atomic_load_explicit (&parameters, std::memory_order_acquire))
        return *p;
    return {};
}

void PadSlot::setParameters (PadParameters newParameters) noexcept
{
    newParameters.sanitise();
    std::atomic_store_explicit (&parameters,
                                std::make_shared<const PadParameters> (newParameters),
                                std::memory_order_release);
}

AurenModel::AurenModel()
{
    for (int flat = 0; flat < kTotalPads; ++flat)
    {
        auto p = pads[static_cast<size_t> (flat)].getParameters();
        p.outputFamily = OutputFamily::main;
        pads[static_cast<size_t> (flat)].setParameters (p);
    }
}

PadSlot& AurenModel::pad (int bank, int padIndex) noexcept { return padFlat (flatPadIndex (bank, padIndex)); }
const PadSlot& AurenModel::pad (int bank, int padIndex) const noexcept { return padFlat (flatPadIndex (bank, padIndex)); }
PadSlot& AurenModel::padFlat (int flatIndex) noexcept { return pads[static_cast<size_t> (juce::jlimit (0, kTotalPads - 1, flatIndex))]; }
const PadSlot& AurenModel::padFlat (int flatIndex) const noexcept { return pads[static_cast<size_t> (juce::jlimit (0, kTotalPads - 1, flatIndex))]; }

void AurenModel::setSelectedBank (int bank) noexcept { selectedBank.store (juce::jlimit (0, kBankCount - 1, bank)); }

bool AurenModel::anySoloed() const noexcept
{
    for (const auto& slot : pads)
        if (slot.getParameters().soloed)
            return true;
    return false;
}

DrumType AurenModel::drumTypeForPad (int flatIndex) const noexcept
{
    if (const auto sampleData = padFlat (flatIndex).getSample())
        return sampleData->type;
    return DrumType::sample;
}

juce::ValueTree AurenModel::toValueTree (bool embedAudio) const
{
    juce::ValueTree root ("AUREN_STATE");
    root.setProperty ("version", 1, nullptr);
    root.setProperty ("selectedBank", getSelectedBank(), nullptr);

    for (int flat = 0; flat < kTotalPads; ++flat)
    {
        juce::ValueTree child ("PAD");
        child.setProperty ("flat", flat, nullptr);
        writeParameters (child, padFlat (flat).getParameters());
        if (const auto sampleData = padFlat (flat).getSample())
        {
            child.setProperty ("name", sampleData->name, nullptr);
            child.setProperty ("path", sampleData->sourcePath, nullptr);
            child.setProperty ("sampleRate", sampleData->sourceSampleRate, nullptr);
            child.setProperty ("drumType", static_cast<int> (sampleData->type), nullptr);
            child.setProperty ("midiNote", sampleData->midiNote, nullptr);
            if (embedAudio)
            {
                child.setProperty ("originalAudio", encodeBuffer (sampleData->original), nullptr);
                child.setProperty ("isolatedAudio", encodeBuffer (sampleData->isolated), nullptr);
            }
        }
        root.addChild (child, -1, nullptr);
    }
    root.addChild (patterns.toValueTree(), -1, nullptr);
    return root;
}

void AurenModel::restoreFromValueTree (const juce::ValueTree& root)
{
    if (! root.hasType ("AUREN_STATE"))
        return;
    setSelectedBank (root.getProperty ("selectedBank", 0));
    for (int i = 0; i < root.getNumChildren(); ++i)
    {
        const auto child = root.getChild (i);
        if (child.hasType ("PATTERNS"))
        {
            patterns.restoreFromValueTree (child);
            continue;
        }
        if (! child.hasType ("PAD"))
            continue;

        const auto flat = juce::jlimit (0, kTotalPads - 1, static_cast<int> (child.getProperty ("flat", 0)));
        padFlat (flat).setParameters (readParameters (child));
        if (! child.hasProperty ("name") && ! child.hasProperty ("path"))
        {
            padFlat (flat).clearSample();
            continue;
        }

        auto data = std::make_shared<PadSample>();
        data->name = child.getProperty ("name").toString();
        data->sourcePath = child.getProperty ("path").toString();
        data->sourceSampleRate = child.getProperty ("sampleRate", 44100.0);
        data->type = static_cast<DrumType> (static_cast<int> (child.getProperty ("drumType", static_cast<int> (DrumType::sample))));
        data->midiNote = child.getProperty ("midiNote", kFirstPadMidiNote + padForFlatIndex (flat));
        data->original = decodeBuffer (child.getProperty ("originalAudio").toString());
        data->isolated = decodeBuffer (child.getProperty ("isolatedAudio").toString());
        padFlat (flat).setSample (std::move (data));
    }
}
} // namespace auren
