#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class MenaceAudioProcessor : public juce::AudioProcessor
{
public:
    MenaceAudioProcessor();
    ~MenaceAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Menace Audio"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Delay  = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;

    double sr = 44100.0;
    float env = 0.f;
    float lastTone = -1.f;

    std::array<Filter, 2> airHP1, airHP2, fbLP;
    Delay delayL { 48000 }, delayR { 48000 };
    juce::SmoothedValue<float> dlySmooth;

    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> wetBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenaceAudioProcessor)
};
