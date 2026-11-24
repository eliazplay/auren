#pragma once

#include "AurenTheme.h"

namespace auren::ui
{
class ThemedComponent
{
public:
    virtual ~ThemedComponent() = default;
    virtual void setTheme (Theme* newTheme) = 0;
};

class AurenButton final : public juce::Button, public ThemedComponent
{
public:
    explicit AurenButton (juce::String text = {});

    void setTheme (Theme*) override;
    void setAccent (bool shouldUseAccent);
    void setCompact (bool shouldBeCompact);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Theme* theme = nullptr;
    bool accent = false;
    bool compact = false;
};

class AurenKnob final : public juce::Component, public ThemedComponent
{
public:
    AurenKnob (juce::String label, juce::NormalisableRange<double> range, double initialValue);

    void setTheme (Theme*) override;
    void setValue (double, juce::NotificationType = juce::sendNotificationAsync);
    [[nodiscard]] double getValue() const noexcept { return value; }
    [[nodiscard]] const juce::String& getLabel() const noexcept { return name; }
    void setAccent (bool);

    std::function<void(double)> onValueChange;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::String name;
    juce::NormalisableRange<double> normalisableRange;
    double value = 0.0;
    double defaultValue = 0.0;
    double valueAtMouseDown = 0.0;
    Theme* theme = nullptr;
    bool accent = false;
};

class AurenSlider final : public juce::Component, public ThemedComponent
{
public:
    AurenSlider (juce::String label, double initialValue = 0.5);

    void setTheme (Theme*) override;
    void setValue (double, juce::NotificationType = juce::sendNotificationAsync);
    [[nodiscard]] double getValue() const noexcept { return value; }
    void setLabelVisible (bool shouldShow) { showLabel = shouldShow; repaint(); }

    std::function<void(double)> onValueChange;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void updateFromMouse (juce::Point<float>);
    juce::String name;
    double value = 0.5;
    Theme* theme = nullptr;
    bool showLabel = true;
};

class SegmentedControl final : public juce::Component, public ThemedComponent
{
public:
    explicit SegmentedControl (juce::StringArray items = {});

    void setTheme (Theme*) override;
    void setItems (juce::StringArray);
    void setSelectedIndex (int, juce::NotificationType = juce::sendNotificationAsync);
    [[nodiscard]] int getSelectedIndex() const noexcept { return selected; }
    void setAccentColour (juce::Colour colour) { accentOverride = colour; repaint(); }

    std::function<void(int)> onSelectionChange;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::StringArray labels;
    int selected = 0;
    Theme* theme = nullptr;
    juce::Colour accentOverride;
};

class AssistantCard final : public juce::Component, public ThemedComponent
{
public:
    AssistantCard();

    void setTheme (Theme*) override;
    void setMessage (juce::String);
    void setPrompt (juce::String);
    [[nodiscard]] juce::String getPrompt() const;

    std::function<void(const juce::String&)> onSubmit;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Theme* theme = nullptr;
    juce::String message;
    juce::TextEditor prompt;
    AurenButton send { ">" };
};

class RackFrame final : public juce::Component, public ThemedComponent
{
public:
    explicit RackFrame (Theme*);

    void setTheme (Theme*) override;
    void paint (juce::Graphics&) override;

private:
    Theme* theme = nullptr;
};
} // namespace auren::ui
