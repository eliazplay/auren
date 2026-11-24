#include "AurenProcessor.h"
#include "AurenEditor.h"

AurenProcessor::AurenProcessor()
 : AudioProcessor (BusesProperties().withOutput ("Main", juce::AudioChannelSet::stereo(), true)),
   apvts (*this, nullptr, "AUREN", createLayout())
{
    formats.registerBasicFormats();
}

juce::AudioProcessorValueTreeState::ParameterLayout AurenProcessor::createLayout()
{
    using namespace auren;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterChoice> (ids::skin, "Skin", juce::StringArray { "Light", "Dark" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ids::masterGain, "Output", juce::NormalisableRange<float> (-24, 12), 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ids::sensitivity, "Sensitivity", 0, 1, .62f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ids::aiSeparation, "AI separation", false));
    return layout;
}

void AurenProcessor::prepareToPlay (double rate, int size) { sampler.prepare(rate,size,getTotalNumOutputChannels()); }
void AurenProcessor::releaseResources() { sampler.reset(); }
bool AurenProcessor::isBusesLayoutSupported (const BusesLayout& l) const { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
void AurenProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    audio.clear();
    auren::SamplerEngine::FamilyOutputs family {};
    sampler.processBlock(audio, family, midi, kitModel.getSelectedBank());
    audio.applyGain(juce::Decibels::decibelsToGain(apvts.getRawParameterValue(auren::ids::masterGain)->load()));
}
void AurenProcessor::getStateInformation (juce::MemoryBlock& data) { auto tree=kitModel.toValueTree(true); tree.addChild(apvts.copyState(),-1,nullptr); juce::MemoryOutputStream out(data,true); tree.writeToStream(out); }
void AurenProcessor::setStateInformation (const void* data,int size) { if(auto tree=juce::ValueTree::readFromData(data,(size_t)size); tree.isValid()) { kitModel.restoreFromValueTree(tree); apvts.replaceState(tree.getChildWithName("AUREN")); } }
void AurenProcessor::loadSource (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file)); if(!reader) return;
    source.setSize((int)reader->numChannels,(int)reader->lengthInSamples); reader->read(&source,0,source.getNumSamples(),0,true,true); sourceRate=reader->sampleRate;
    hits=auren::dsp::DrumAnalyzer::analyze(source,sourceRate,{apvts.getRawParameterValue(auren::ids::sensitivity)->load()});
}
void AurenProcessor::cloneSource()
{
    std::map<int,std::vector<auren::dsp::HitDescriptor>> groups; for(auto& h:hits) groups[h.groupId].push_back(h);
    int i=0; for(auto& [id, group]:groups) { if(i>=auren::kPadsPerBank) break; auto x=auren::dsp::CleanHitExtractor::extractGroup(source,sourceRate,group,id); if(!x.isValid())continue; auto s=std::make_shared<auren::PadSample>();s->original=x.audio;s->isolated=x.audio;s->sourceSampleRate=x.sampleRate;s->name=auren::dsp::familyName(x.family); kitModel.pad(0,i++).setSample(s); }
}
juce::AudioProcessorEditor* AurenProcessor::createEditor() { return new auren::AurenEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AurenProcessor(); }
