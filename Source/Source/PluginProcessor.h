#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace MA
{
    enum ParamIndex
    {
        pGain, pHpf, pTame, pGate,
        pDeEss, pComp, pSpd, pSat,
        pAir, pFreq, pDrive, pSpark,
        pDlyMix, pDlyTime, pDlyFb, pDlyDark,
        pRevMix, pRevSize, pRevPre, pRevDamp,
        pWidth, pMix, pOut,
        numParams
    };

    inline const char* const paramIds[numParams] = {
        "gain", "hpf", "tame", "gate",
        "deess", "comp", "speed", "warmth",
        "air", "airFreq", "airDrive", "airSpark",
        "dlyMix", "dlyTime", "dlyFb", "dlyDark",
        "revMix", "revSize", "revPre", "revDamp",
        "width", "mix", "output" };

    constexpr int numPresets = 6;
}

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
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return MA::numPresets; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using FPair  = std::array<Filter, 2>;
    using Delay  = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;

    double sr = 44100.0;
    int currentPreset = 0;

    // estado dos detectores
    float env = 0.f, gEnv = 0.f, gGain = 1.f, dsEnv = 0.f;
    bool gateOpen = false;

    // cache para só recalcular filtros quando o knob muda
    int lastHpf = -1;
    float lastTame = -1.f, lastAirFreq = -1.f, lastAirAmt = -1.f, lastDark = -1.f;

    FPair hpf1, hpf2, tameF, dsHP, airHP1, airHP2, airHP3, airShelf, fbLP, fbHP, sendHP;
    Delay delayL { 96000 }, delayR { 96000 }, preL { 16000 }, preR { 16000 };

    juce::SmoothedValue<float> inGainSm, outGainSm, mixSm, dlySm;
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> dlyBuf, rvBuf, origBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenaceAudioProcessor)
};
