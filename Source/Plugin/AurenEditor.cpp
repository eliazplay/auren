#include "AurenEditor.h"

#include <map>

namespace auren
{
AurenEditor::AurenEditor (::AurenProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    setResizable (true, true); setResizeLimits (1040, 650, 2400, 1500); setSize (1584, 992);
    addAndMakeVisible (view);
    ui::AurenMainView::Callbacks cb;
    cb.importAudio = [this] { chooseSource(); }; cb.browseAudio = [this] { chooseSource(); };
    cb.analyseWithAI = [this] { processor.cloneSource(); refresh(); };
    cb.reclone = [this] { processor.cloneSource(); refresh(); };
    cb.selectPad = [this] (int) { refresh(); };
    cb.triggerPad = [this] (int index, float vel) { processor.model().setSelectedBank(index / kPadsPerBank); };
    cb.loadPad = [this] { choosePad(); };
    cb.assistantMessage = [this] (const juce::String& s) { auto r=processor.assistant().handle(s,processor.model(),view.getPadGrid().getSelectedPad()%kPadsPerBank); view.setAssistantMessage(r.message); refresh(); };
    view.setCallbacks (std::move(cb));
    view.setSkin (processor.parameters().getRawParameterValue(ids::skin)->load() > .5f ? ui::Skin::dark : ui::Skin::light);
    refresh(); startTimerHz (12);
}
void AurenEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }
void AurenEditor::resized() { view.setBounds (getLocalBounds()); }
std::vector<float> AurenEditor::mono (const juce::AudioBuffer<float>& b)
{ std::vector<float> r((size_t)b.getNumSamples()); for(int i=0;i<b.getNumSamples();++i){float x=0;for(int c=0;c<b.getNumChannels();++c)x+=b.getSample(c,i);r[(size_t)i]=x/(float)juce::jmax(1,b.getNumChannels());} return r; }
void AurenEditor::refresh()
{
    const auto& src=processor.sourceAudio(); std::vector<ui::HitMarker> markers; std::vector<ui::FamilyCount> counts;
    std::map<dsp::DrumFamily,int> n; for(auto& h:processor.sourceHits()){auto fam=dsp::familyName(h.family);markers.push_back({(float)h.samplePosition/(float)juce::jmax(1,src.getNumSamples()),fam}); ++n[h.family];}
    for(auto& [fam,count]:n) counts.push_back({dsp::familyName(fam),count});
    view.setSourceAudio(mono(src),markers,counts,src.getNumSamples()?"Loaded source":"Drop audio to begin",src.getNumSamples()?"Ready":"",(int)processor.sourceHits().size());
    std::array<ui::PadVisualState,64> pads; for(int i=0;i<64;++i){auto sm=processor.model().padFlat(i).getSample();if(sm){pads[(size_t)i].occupied=true;pads[(size_t)i].name=sm->name;pads[(size_t)i].family=drumTypeName(sm->type);pads[(size_t)i].waveform=mono(sm->isolated);}}
    view.setPads(std::move(pads)); auto selected=view.getPadGrid().getSelectedPad(); auto sm=processor.model().padFlat(selected).getSample(); if(sm) view.setSelectedPadAudio(selected,sm->name,drumTypeName(sm->type),mono(sm->isolated),1,1);
}
void AurenEditor::timerCallback(){ auto expected=processor.parameters().getRawParameterValue(ids::skin)->load()>.5f?ui::Skin::dark:ui::Skin::light; if(view.getSkin()!=expected)view.setSkin(expected); }
void AurenEditor::chooseSource(){ chooser=std::make_unique<juce::FileChooser>("Load source",{},"*.wav;*.aif;*.aiff;*.flac;*.mp3"); chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c){auto f=c.getResult();if(f.existsAsFile()){processor.loadSource(f);refresh();}}); }
void AurenEditor::choosePad(){ chooseSource(); }
} // namespace auren
