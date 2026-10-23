#pragma once
#include "PluginProcessor.h"

// Knob de porcelana com ponteiro de madeira clara
class MenaceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;
};

class MenaceAudioEditor : public juce::AudioProcessorEditor
{
public:
    explicit MenaceAudioEditor (MenaceAudioProcessor&);
    ~MenaceAudioEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int numKnobs = 9;

    MenaceAudioProcessor& proc;
    MenaceLookAndFeel laf;

    std::array<juce::Slider, numKnobs> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numKnobs> attachments;
    std::array<juce::Rectangle<int>, 4> cards;
    std::array<juce::Rectangle<int>, numKnobs> knobRects;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenaceAudioEditor)
};
