#pragma once
#include "PluginProcessor.h"

// Knob de porcelana serrilhado, base de madeira clara e travas (detents) marcadas por pontos
class MenaceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MenaceLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

class MenaceAudioEditor : public juce::AudioProcessorEditor
{
public:
    explicit MenaceAudioEditor (MenaceAudioProcessor&);
    ~MenaceAudioEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MenaceAudioProcessor& proc;
    MenaceLookAndFeel laf;

    std::array<juce::Slider, MA::numParams> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, MA::numParams> attachments;
    std::array<juce::Rectangle<int>, 6> cards;
    std::array<juce::Rectangle<int>, MA::numParams> knobRects;
    juce::ComboBox presetBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenaceAudioEditor)
};
