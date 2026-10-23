#include "PluginProcessor.h"
#include "PluginEditor.h"

// Cadeia: Compressor -> Air (harmônicos) -> Delay ping-pong -> Reverb -> Output

juce::AudioProcessorValueTreeState::ParameterLayout MenaceAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&] (const char* id, const char* name, R range, float def)
    {
        layout.add (std::make_unique<P> (juce::ParameterID { id, 1 }, name, range, def));
    };

    add ("compAmt",  "Comp Amount", R (0.f, 1.f, 0.001f), 0.40f);
    add ("compSpd",  "Comp Speed",  R (0.f, 1.f, 0.001f), 0.50f);
    add ("airAmt",   "Air",         R (0.f, 1.f, 0.001f), 0.35f);
    add ("airTone",  "Air Tone",    R (0.f, 1.f, 0.001f), 0.50f);
    add ("dlyMix",   "Delay Mix",   R (0.f, 1.f, 0.001f), 0.15f);
    add ("dlyTime",  "Delay Time",  R (60.f, 600.f, 0.1f, 0.6f), 250.f);
    add ("revMix",   "Reverb Mix",  R (0.f, 1.f, 0.001f), 0.20f);
    add ("revSize",  "Reverb Size", R (0.f, 1.f, 0.001f), 0.50f);
    add ("output",   "Output",      R (-12.f, 12.f, 0.1f), 0.f);
    return layout;
}

MenaceAudioProcessor::MenaceAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool MenaceAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void MenaceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec mono   { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    juce::dsp::ProcessSpec stereo { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    for (auto* d : { &delayL, &delayR })
    {
        d->setMaximumDelayInSamples ((int) (sampleRate * 0.7));
        d->prepare (mono);
        d->reset();
    }

    reverb.prepare (stereo);
    reverb.reset();
    wetBuf.setSize (2, juce::jmax (1, samplesPerBlock));

    auto lp = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 4500.f, 0.707f);
    for (int c = 0; c < 2; ++c)
    {
        airHP1[(size_t) c].reset();
        airHP2[(size_t) c].reset();
        fbLP[(size_t) c].reset();
        fbLP[(size_t) c].coefficients = lp;
    }

    lastTone = -1.f;
    env = 0.f;
    dlySmooth.reset (sampleRate, 0.15);
    dlySmooth.setCurrentAndTargetValue ((float) (0.25 * sampleRate));
}

void MenaceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0) return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const float compAmt = get ("compAmt"), compSpd = get ("compSpd");
    const float airAmt  = get ("airAmt"),  airTone = get ("airTone");
    const float dlyMix  = get ("dlyMix"),  dlyTime = get ("dlyTime");
    const float revMix  = get ("revMix"),  revSize = get ("revSize");
    const float outDb   = get ("output");

    // --- compressor (1 knob de quantidade, 1 de velocidade) ---
    const float thr    = -2.f - compAmt * 28.f;
    const float ratio  = 1.f + compAmt * 5.f;
    const float atkMs  = juce::jmap (compSpd, 30.f, 2.f);
    const float relMs  = juce::jmap (compSpd, 250.f, 60.f);
    const float atkC   = std::exp (-1.f / ((float) sr * atkMs * 0.001f));
    const float relC   = std::exp (-1.f / ((float) sr * relMs * 0.001f));
    const float makeup = -thr * (1.f - 1.f / ratio) * 0.5f;

    // --- air: coeficientes só quando o Tone muda ---
    if (std::abs (airTone - lastTone) > 0.001f)
    {
        const float f = juce::jmap (airTone, 3500.f, 11000.f);
        auto c = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, f, 0.707f);
        for (size_t ch = 0; ch < 2; ++ch) { airHP1[ch].coefficients = c; airHP2[ch].coefficients = c; }
        lastTone = airTone;
    }

    dlySmooth.setTargetValue (dlyTime * 0.001f * (float) sr);

    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { l[i], r[i] };

        // compressor (stereo linkado)
        const float lvl = juce::jmax (std::abs (x[0]), std::abs (x[1]));
        env = lvl > env ? atkC * env + (1.f - atkC) * lvl
                        : relC * env + (1.f - relC) * lvl;
        const float db   = 20.f * std::log10 (juce::jmax (env, 1.0e-6f));
        const float over = db - thr;
        const float gr   = over > 0.f ? over * (1.f - 1.f / ratio) : 0.f;
        const float g    = juce::Decibels::decibelsToGain (makeup - gr);
        x[0] *= g; x[1] *= g;

        // air: satura a banda alta (harmônicos pares + ímpares) e soma de volta
        for (size_t c = 0; c < 2; ++c)
        {
            const float hp = airHP1[c].processSample (x[c]);
            const float s  = std::tanh (10.f * hp + 0.4f) - 0.3799f;
            const float h  = airHP2[c].processSample (s);
            x[c] += airAmt * (0.45f * hp + 0.15f * h);
        }

        // delay ping-pong com filtro passa-baixa no feedback
        const float d  = dlySmooth.getNextValue();
        const float dl = delayL.popSample (0, d, true);
        const float dr = delayR.popSample (0, d, true);
        delayL.pushSample (0, x[0] + fbLP[0].processSample (dr) * 0.38f);
        delayR.pushSample (0, x[1] + fbLP[1].processSample (dl) * 0.38f);

        l[i] = x[0] + dl * dlyMix * 0.7f;
        r[i] = x[1] + dr * dlyMix * 0.7f;
    }

    // --- reverb (envio paralelo) ---
    if (wetBuf.getNumSamples() < n) wetBuf.setSize (2, n, false, false, true);
    for (int c = 0; c < 2; ++c) wetBuf.copyFrom (c, 0, buffer, c, 0, n);

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.30f + 0.65f * revSize;
    rp.damping  = 0.55f;
    rp.wetLevel = 1.f;
    rp.dryLevel = 0.f;
    rp.width    = 1.f;
    reverb.setParameters (rp);

    juce::dsp::AudioBlock<float> block (wetBuf);
    auto sub = block.getSubBlock (0, (size_t) n);
    juce::dsp::ProcessContextReplacing<float> ctx (sub);
    reverb.process (ctx);

    for (int c = 0; c < 2; ++c) buffer.addFrom (c, 0, wetBuf, c, 0, n, revMix * 0.45f);

    buffer.applyGain (juce::Decibels::decibelsToGain (outDb));
}

juce::AudioProcessorEditor* MenaceAudioProcessor::createEditor()
{
    return new MenaceAudioEditor (*this);
}

void MenaceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MenaceAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MenaceAudioProcessor();
}
