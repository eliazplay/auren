#pragma once

#include "../Engine/AurenModel.h"

#include <juce_core/juce_core.h>

namespace auren
{
struct AssistantReply
{
    juce::String message;
    bool changedKit = false;
    bool changedPattern = false;
};

// A fast, offline command assistant. It intentionally performs a small set of
// predictable production operations instead of sending session audio to a
// remote service. A model-backed intent provider can be connected later while
// keeping these guarded editing operations unchanged.
class AurenAssistant
{
public:
    AssistantReply handle (juce::String request,
                           AurenModel& model,
                           int selectedPad,
                           std::uint32_t variationSeed = 0x41555245u) const;

private:
    static bool containsAny (const juce::String& text,
                             std::initializer_list<const char*> terms);
    static VariationScope inferScope (const juce::String& text);
};
} // namespace auren
