#include "AurenAssistant.h"

namespace auren
{
bool AurenAssistant::containsAny (const juce::String& text,
                                  std::initializer_list<const char*> terms)
{
    for (const auto* term : terms)
        if (text.containsIgnoreCase (term))
            return true;

    return false;
}

VariationScope AurenAssistant::inferScope (const juce::String& text)
{
    if (containsAny (text, { "kick", "bombo" }))
        return VariationScope::kick;
    if (containsAny (text, { "snare", "caja", "clap" }))
        return VariationScope::snare;
    if (containsAny (text, { "hat", "hihat", "hi-hat", "plato" }))
        return VariationScope::hats;
    return VariationScope::all;
}

AssistantReply AurenAssistant::handle (juce::String request,
                                       AurenModel& model,
                                       int selectedPad,
                                       std::uint32_t variationSeed) const
{
    request = request.trim().toLowerCase();
    if (request.isEmpty())
        return { "Escribe qué quieres cambiar en el kit.", false, false };

    const auto bank = model.getSelectedBank();
    auto& slot = model.pad (bank, juce::jlimit (0, kPadsPerBank - 1, selectedPad));
    auto parameters = slot.getParameters();

    if (containsAny (request, { "limpio", "limpia", "clean", "frecuencia rara",
                                "rare frequency", "rumble", "subgrave" }))
    {
        parameters.clean = 1.0f;
        parameters.highPassHz = juce::jmax (parameters.highPassHz, 24.0f);
        parameters.lowPassHz = juce::jmin (parameters.lowPassHz, 18000.0f);
        parameters.fadeMs = juce::jmax (parameters.fadeMs, 24.0f);
        parameters.sanitise();
        slot.setParameters (parameters);
        return { "He limpiado el pad seleccionado, protegido el subgrave y suavizado la cola.",
                 true, false };
    }

    if (containsAny (request, { "más fuerte", "mas fuerte", "louder", "sube" }))
    {
        parameters.gainDb = juce::jlimit (-60.0f, 12.0f, parameters.gainDb + 2.0f);
        slot.setParameters (parameters);
        return { "He subido el pad 2 dB manteniendo margen en la salida.", true, false };
    }

    if (containsAny (request, { "más corto", "mas corto", "shorter", "recorta" }))
    {
        parameters.fadeMs = juce::jlimit (5.0f, 10000.0f, parameters.fadeMs * 0.68f);
        slot.setParameters (parameters);
        return { "He acortado la cola del pad seleccionado.", true, false };
    }

    if (containsAny (request, { "variación", "variacion", "variation", "vary",
                                "roll", "fill", "relleno" }))
    {
        auto variation = model.getPatterns().current().createVariation (
            inferScope (request), 0.55f, model, variationSeed);
        model.getPatterns().add (std::move (variation));
        model.getPatterns().setCurrentIndex (model.getPatterns().size() - 1);
        return { "He creado una variación editable y conservado el patrón original.",
                 false, true };
    }

    if (containsAny (request, { "pan", "izquierda", "left" }))
    {
        parameters.pan = -0.35f;
        slot.setParameters (parameters);
        return { "He movido el pad ligeramente a la izquierda.", true, false };
    }

    if (containsAny (request, { "derecha", "right" }))
    {
        parameters.pan = 0.35f;
        slot.setParameters (parameters);
        return { "He movido el pad ligeramente a la derecha.", true, false };
    }

    return { "Puedo limpiar o acortar un pad, ajustar nivel y panorama, o crear variaciones de kick, snare y hats.",
             false, false };
}
} // namespace auren
