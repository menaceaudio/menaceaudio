#include "PluginProcessor.h"
#include "PluginEditor.h"

// Cadeia:
// Gain -> HPF -> Tame -> Gate -> De-Esser -> Compressor -> Warmth -> AIR
//      -> Delay ping-pong (sync) -> Reverb (pre-delay) -> Width -> Mix -> Output + limiter suave

namespace
{
    struct Preset { const char* name; float v[MA::numParams]; };

    //                gain hpf tame gate dess comp spd sat  air frq drv spk  dMix dTim dFb dDrk  rMix rSz rPre rDmp  wid mix out
    const Preset presets[MA::numPresets] = {
        { "Init",         { 0, 2, 2, 0,   4, 6, 5, 3,   6, 6, 5, 3,   3, 3, 4, 5,   3, 7, 3, 5,   7, 10,  0 } },
        { "Utopia",       { 0, 2, 3, 0,   5, 7, 6, 4,   8, 7, 7, 5,   4, 3, 5, 6,   5, 9, 4, 4,   9, 10,  0 } },
        { "Toliver Silk", { 0, 2, 3, 0,   5, 6, 4, 5,   5, 5, 4, 2,   3, 2, 3, 4,   4, 8, 3, 6,   7, 10,  0 } },
        { "Dark Throw",   { 0, 3, 2, 0,   4, 7, 5, 6,   5, 4, 4, 2,   6, 3, 7, 7,   6, 9, 5, 7,   9, 10, -1 } },
        { "Clean Booth",  { 0, 2, 4, 2,   6, 5, 5, 1,   4, 6, 3, 1,   1, 2, 2, 3,   2, 5, 2, 5,   5, 10,  0 } },
        { "Air Maluco",   { 0, 2, 3, 0,   5, 7, 6, 3,  10, 8, 9, 8,   3, 3, 4, 5,   4, 8, 3, 3,  10, 10, -2 } },
    };

    inline float softLimit (float y)
    {
        const float t = 0.8f;
        const float a = std::abs (y);
        if (a <= t) return y;
        const float o = t + (1.f - t) * std::tanh ((a - t) / (1.f - t));
        return y < 0.f ? -o : o;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MenaceAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&] (int idx, const char* name, float lo, float hi, float step, float def)
    {
        auto attr = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int)
            {
                return std::abs (v - std::round (v)) < 0.01f ? juce::String ((int) std::round (v))
                                                              : juce::String (v, 1);
            });
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { MA::paramIds[idx], 1 }, name,
            juce::NormalisableRange<float> (lo, hi, step), def, attr));
    };
    auto addC = [&] (int idx, const char* name, juce::StringArray items, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { MA::paramIds[idx], 1 }, name, items, def));
    };

    addF (MA::pGain,    "Input Gain",   -12.f, 12.f, 1.f, 0.f);
    addC (MA::pHpf,     "HPF",          { "Off", "60", "80", "100", "120", "150" }, 2);
    addF (MA::pTame,    "Tame",         0.f, 10.f, 0.5f, 2.f);
    addF (MA::pGate,    "Gate",         0.f, 10.f, 0.5f, 0.f);

    addF (MA::pDeEss,   "De-Ess",       0.f, 10.f, 0.5f, 4.f);
    addF (MA::pComp,    "Comp",         0.f, 10.f, 0.5f, 6.f);
    addF (MA::pSpd,     "Comp Speed",   0.f, 10.f, 0.5f, 5.f);
    addF (MA::pSat,     "Warmth",       0.f, 10.f, 0.5f, 3.f);

    addF (MA::pAir,     "Air",          0.f, 10.f, 0.5f, 6.f);
    addF (MA::pFreq,    "Air Freq",     0.f, 10.f, 0.5f, 6.f);
    addF (MA::pDrive,   "Air Drive",    0.f, 10.f, 0.5f, 5.f);
    addF (MA::pSpark,   "Air Spark",    0.f, 10.f, 0.5f, 3.f);

    addF (MA::pDlyMix,  "Delay Mix",    0.f, 10.f, 0.5f, 3.f);
    addC (MA::pDlyTime, "Delay Time",   { "1/4", "1/4 D", "1/8", "1/8 D", "1/16", "1/8 T" }, 3);
    addF (MA::pDlyFb,   "Delay Feedback", 0.f, 10.f, 0.5f, 4.f);
    addF (MA::pDlyDark, "Delay Dark",   0.f, 10.f, 0.5f, 5.f);

    addF (MA::pRevMix,  "Reverb Mix",   0.f, 10.f, 0.5f, 3.f);
    addF (MA::pRevSize, "Reverb Size",  0.f, 10.f, 0.5f, 7.f);
    addF (MA::pRevPre,  "Reverb Pre",   0.f, 10.f, 0.5f, 3.f);
    addF (MA::pRevDamp, "Reverb Damp",  0.f, 10.f, 0.5f, 5.f);

    addF (MA::pWidth,   "Width",        0.f, 10.f, 0.5f, 7.f);
    addF (MA::pMix,     "Mix",          0.f, 10.f, 0.5f, 10.f);
    addF (MA::pOut,     "Output",       -12.f, 12.f, 1.f, 0.f);
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

void MenaceAudioProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, MA::numPresets - 1, index);
    currentPreset = index;
    for (int k = 0; k < MA::numParams; ++k)
        if (auto* prm = apvts.getParameter (MA::paramIds[k]))
            prm->setValueNotifyingHost (prm->convertTo0to1 (presets[index].v[k]));
}

const juce::String MenaceAudioProcessor::getProgramName (int index)
{
    return presets[juce::jlimit (0, MA::numPresets - 1, index)].name;
}

void MenaceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const auto blk = (juce::uint32) juce::jmax (1, samplesPerBlock);
    juce::dsp::ProcessSpec mono   { sampleRate, blk, 1 };
    juce::dsp::ProcessSpec stereo { sampleRate, blk, 2 };

    delayL.setMaximumDelayInSamples ((int) (sampleRate * 2.0));
    delayR.setMaximumDelayInSamples ((int) (sampleRate * 2.0));
    preL.setMaximumDelayInSamples ((int) (sampleRate * 0.2));
    preR.setMaximumDelayInSamples ((int) (sampleRate * 0.2));
    for (auto* d : { &delayL, &delayR, &preL, &preR }) { d->prepare (mono); d->reset(); }

    reverb.prepare (stereo);
    reverb.reset();

    const int nb = (int) blk;
    dlyBuf.setSize (2, nb);
    rvBuf.setSize (2, nb);
    origBuf.setSize (2, nb);

    auto setBoth = [] (FPair& f, Coeffs::Ptr c) { f[0].coefficients = c; f[1].coefficients = c; f[0].reset(); f[1].reset(); };
    setBoth (dsHP,   Coeffs::makeHighPass (sampleRate, 5500.f, 0.707f));
    setBoth (fbHP,   Coeffs::makeHighPass (sampleRate, 180.f, 0.707f));
    setBoth (sendHP, Coeffs::makeHighPass (sampleRate, 250.f, 0.707f));
    for (auto* f : { &hpf1, &hpf2, &tameF, &airHP1, &airHP2, &airHP3, &airShelf, &fbLP })
    { (*f)[0].reset(); (*f)[1].reset(); }

    lastHpf = -1; lastTame = lastAirFreq = lastAirAmt = lastDark = -1.f;
    env = gEnv = dsEnv = 0.f; gGain = 1.f; gateOpen = false;

    inGainSm.reset (sampleRate, 0.05);  inGainSm.setCurrentAndTargetValue (1.f);
    outGainSm.reset (sampleRate, 0.05); outGainSm.setCurrentAndTargetValue (1.f);
    mixSm.reset (sampleRate, 0.05);     mixSm.setCurrentAndTargetValue (1.f);
    dlySm.reset (sampleRate, 0.12);     dlySm.setCurrentAndTargetValue ((float) (0.25 * sampleRate));
}

void MenaceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0) return;

    auto P = [this] (int i) { return apvts.getRawParameterValue (MA::paramIds[i])->load(); };
    const float inDb    = P (MA::pGain);
    const int   hpfIdx  = (int) P (MA::pHpf);
    const float tameV   = P (MA::pTame) / 10.f;
    const float gateV   = P (MA::pGate) / 10.f;
    const float deEssV  = P (MA::pDeEss) / 10.f;
    const float compAmt = P (MA::pComp) / 10.f;
    const float compSpd = P (MA::pSpd) / 10.f;
    const float sat     = P (MA::pSat) / 10.f;
    const float airAmt  = P (MA::pAir) / 10.f;
    const float airFreqN = P (MA::pFreq) / 10.f;
    const float airDrv  = P (MA::pDrive) / 10.f;
    const float spark   = P (MA::pSpark) / 10.f;
    const float dlyMix  = P (MA::pDlyMix) / 10.f;
    const int   dlyIdx  = juce::jlimit (0, 5, (int) P (MA::pDlyTime));
    const float dlyFb   = P (MA::pDlyFb) / 10.f;
    const float dlyDark = P (MA::pDlyDark) / 10.f;
    const float revMix  = P (MA::pRevMix) / 10.f;
    const float revSize = P (MA::pRevSize) / 10.f;
    const float revPre  = P (MA::pRevPre) / 10.f;
    const float revDamp = P (MA::pRevDamp) / 10.f;
    const float width   = P (MA::pWidth) / 10.f;
    const float mix     = P (MA::pMix) / 10.f;
    const float outDb   = P (MA::pOut);

    if (dlyBuf.getNumSamples() < n)
    {
        dlyBuf.setSize (2, n, false, false, true);
        rvBuf.setSize (2, n, false, false, true);
        origBuf.setSize (2, n, false, false, true);
    }
    for (int c = 0; c < 2; ++c) origBuf.copyFrom (c, 0, buffer, c, 0, n);

    // ---------- coeficientes (só quando o knob muda) ----------
    auto setBoth = [] (FPair& f, Coeffs::Ptr c) { f[0].coefficients = c; f[1].coefficients = c; };
    const float nyq = 0.45f * (float) sr;

    if (hpfIdx != lastHpf)
    {
        static const float fr[] = { 0.f, 60.f, 80.f, 100.f, 120.f, 150.f };
        if (hpfIdx > 0)
        {
            auto c = Coeffs::makeHighPass (sr, fr[hpfIdx], 0.707f);
            setBoth (hpf1, c); setBoth (hpf2, c);
        }
        lastHpf = hpfIdx;
    }
    if (std::abs (tameV - lastTame) > 0.001f)
    {
        // domar a região de presença do condensador (~6.5 kHz), até -5 dB
        setBoth (tameF, Coeffs::makePeakFilter (sr, 6500.f, 1.1f, juce::Decibels::decibelsToGain (-tameV * 5.f)));
        lastTame = tameV;
    }
    if (std::abs (airFreqN - lastAirFreq) > 0.001f || std::abs (airAmt - lastAirAmt) > 0.001f)
    {
        const float f = juce::jmin (juce::jmap (airFreqN, 3500.f, 14000.f), nyq);
        auto hp = Coeffs::makeHighPass (sr, f, 0.707f);
        setBoth (airHP1, hp); setBoth (airHP2, hp);
        setBoth (airHP3, Coeffs::makeHighPass (sr, juce::jmin (f * 1.4f, nyq), 0.707f));
        setBoth (airShelf, Coeffs::makeHighShelf (sr, juce::jmin (f * 0.7f, nyq), 0.7f,
                                                  juce::Decibels::decibelsToGain (airAmt * 10.f)));
        lastAirFreq = airFreqN; lastAirAmt = airAmt;
    }
    if (std::abs (dlyDark - lastDark) > 0.001f)
    {
        setBoth (fbLP, Coeffs::makeLowPass (sr, juce::jmin (10000.f - dlyDark * 8500.f, nyq), 0.707f));
        lastDark = dlyDark;
    }

    // ---------- constantes do bloco ----------
    inGainSm.setTargetValue (juce::Decibels::decibelsToGain (inDb));
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (outDb));
    mixSm.setTargetValue (mix);

    // gate
    const float gateThr = juce::Decibels::decibelsToGain (-80.f + gateV * 45.f);
    const float gRel    = std::exp (-1.f / (0.03f * (float) sr));
    const float gAtk    = 1.f - std::exp (-1.f / (0.002f * (float) sr));
    const float gClose  = 1.f - std::exp (-1.f / (0.12f * (float) sr));

    // de-esser
    const float deThr = -8.f - deEssV * 26.f;
    const float atkD  = std::exp (-1.f / (0.0003f * (float) sr));
    const float relD  = std::exp (-1.f / (0.05f * (float) sr));

    // compressor
    const float thr    = -2.f - compAmt * 30.f;
    const float ratio  = 1.f + compAmt * 7.f;
    const float atkC   = std::exp (-1.f / ((float) sr * juce::jmap (compSpd, 30.f, 2.f) * 0.001f));
    const float relC   = std::exp (-1.f / ((float) sr * juce::jmap (compSpd, 250.f, 60.f) * 0.001f));
    const float makeup = -thr * (1.f - 1.f / ratio) * 0.45f;

    // warmth
    const float satD = 1.f + sat * 5.f, satB = 0.12f * sat, satT = std::tanh (satB);

    // air
    const float drv = 3.f + airDrv * 22.f;

    // delay (sync com o BPM do FL Studio)
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = juce::jlimit (20.0, 400.0, *b);
    static const float beats[] = { 1.f, 1.5f, 0.5f, 0.75f, 0.25f, 1.f / 3.f };
    const float dMax = (float) delayL.getMaximumDelayInSamples() - 2.f;
    dlySm.setTargetValue (juce::jmin ((float) (60.0 / bpm * beats[dlyIdx] * sr), dMax));
    const float fb = dlyFb * 0.82f;

    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);
    auto* dL = dlyBuf.getWritePointer (0);
    auto* dR = dlyBuf.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float ig = inGainSm.getNextValue();
        float x[2] = { l[i] * ig, r[i] * ig };

        // HPF 24 dB/oct
        if (hpfIdx > 0)
            for (size_t c = 0; c < 2; ++c) x[c] = hpf2[c].processSample (hpf1[c].processSample (x[c]));

        // Tame
        if (tameV > 0.f)
            for (size_t c = 0; c < 2; ++c) x[c] = tameF[c].processSample (x[c]);

        // Gate suave
        if (gateV > 0.f)
        {
            const float lv = juce::jmax (std::abs (x[0]), std::abs (x[1]));
            gEnv = lv > gEnv ? lv : gEnv * gRel;
            if (! gateOpen && gEnv > gateThr) gateOpen = true;
            else if (gateOpen && gEnv < gateThr * 0.5f) gateOpen = false;
            gGain += ((gateOpen ? 1.f : 0.1f) - gGain) * (gateOpen ? gAtk : gClose);
            x[0] *= gGain; x[1] *= gGain;
        }

        // De-esser (reduz só a banda acima de 5.5 kHz)
        {
            const float hi0 = dsHP[0].processSample (x[0]);
            const float hi1 = dsHP[1].processSample (x[1]);
            if (deEssV > 0.f)
            {
                const float lv = juce::jmax (std::abs (hi0), std::abs (hi1));
                dsEnv = lv > dsEnv ? atkD * dsEnv + (1.f - atkD) * lv
                                   : relD * dsEnv + (1.f - relD) * lv;
                const float over = 20.f * std::log10 (juce::jmax (dsEnv, 1.0e-6f)) - deThr;
                const float gr = over > 0.f ? juce::jmin (over * 0.75f, 14.f) : 0.f;
                const float gd = juce::Decibels::decibelsToGain (-gr);
                x[0] = (x[0] - hi0) + hi0 * gd;
                x[1] = (x[1] - hi1) + hi1 * gd;
            }
        }

        // Compressor (stereo linkado)
        {
            const float lv = juce::jmax (std::abs (x[0]), std::abs (x[1]));
            env = lv > env ? atkC * env + (1.f - atkC) * lv : relC * env + (1.f - relC) * lv;
            const float over = 20.f * std::log10 (juce::jmax (env, 1.0e-6f)) - thr;
            const float gr = over > 0.f ? over * (1.f - 1.f / ratio) : 0.f;
            const float g = juce::Decibels::decibelsToGain (makeup - gr);
            x[0] *= g; x[1] *= g;
        }

        // Warmth (saturação assimétrica)
        if (sat > 0.f)
            for (size_t c = 0; c < 2; ++c)
                x[c] = (std::tanh (satD * x[c] + satB) - satT) / satD;

        // AIR: shelf + harmônicos saturados + octave-up "spark"
        if (airAmt > 0.f || spark > 0.f)
        {
            for (size_t c = 0; c < 2; ++c)
            {
                const float xs = airShelf[c].processSample (x[c]);
                const float hp = airHP1[c].processSample (x[c]);
                const float h  = airHP2[c].processSample (std::tanh (drv * hp + 0.4f) - 0.3799f);
                const float sp = airHP3[c].processSample (std::abs (hp) * 3.f);
                x[c] = xs + airAmt * (0.6f * hp + 0.22f * h) + spark * 0.6f * sp;
            }
        }

        l[i] = x[0];
        r[i] = x[1];

        // Delay ping-pong (entrada mono, feedback escurecido)
        const float d  = dlySm.getNextValue();
        const float dl = delayL.popSample (0, d, true);
        const float dr = delayR.popSample (0, d, true);
        delayL.pushSample (0, 0.5f * (x[0] + x[1]) + fb * fbHP[0].processSample (fbLP[0].processSample (dr)));
        delayR.pushSample (0, fbHP[1].processSample (fbLP[1].processSample (dl)));
        dL[i] = dl;
        dR[i] = dr;
    }

    // ---------- reverb: recebe o vocal + o delay, com pre-delay ----------
    const float dlyG = dlyMix * 0.8f;
    const float revG = revMix * 0.55f;
    const float preSamp = juce::jmax (1.f, revPre * 0.12f * (float) sr);

    for (int c = 0; c < 2; ++c)
    {
        rvBuf.copyFrom (c, 0, buffer, c, 0, n);
        rvBuf.addFrom (c, 0, dlyBuf, c, 0, n, dlyG);
    }
    {
        auto* rl = rvBuf.getWritePointer (0);
        auto* rr = rvBuf.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float sl = sendHP[0].processSample (rl[i]);
            const float sr_ = sendHP[1].processSample (rr[i]);
            rl[i] = preL.popSample (0, preSamp, true);
            rr[i] = preR.popSample (0, preSamp, true);
            preL.pushSample (0, sl);
            preR.pushSample (0, sr_);
        }
    }

    juce::Reverb::Parameters rp;
    rp.roomSize = juce::jmin (0.3f + 0.69f * revSize, 0.99f);
    rp.damping  = 0.2f + 0.7f * revDamp;
    rp.wetLevel = 1.f;
    rp.dryLevel = 0.f;
    rp.width    = 1.f;
    reverb.setParameters (rp);

    juce::dsp::AudioBlock<float> block (rvBuf);
    auto sub = block.getSubBlock (0, (size_t) n);
    juce::dsp::ProcessContextReplacing<float> ctx (sub);
    reverb.process (ctx);

    // ---------- retornos + width + mix + saída ----------
    const float wFac = width * 2.f;
    const auto* rl = rvBuf.getReadPointer (0);
    const auto* rr = rvBuf.getReadPointer (1);
    const auto* ol = origBuf.getReadPointer (0);
    const auto* orr = origBuf.getReadPointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float retL = dL[i] * dlyG + rl[i] * revG;
        const float retR = dR[i] * dlyG + rr[i] * revG;
        const float m = 0.5f * (retL + retR);
        const float s = 0.5f * (retL - retR) * wFac;

        const float mx = mixSm.getNextValue();
        const float og = outGainSm.getNextValue();
        l[i] = softLimit ((ol[i]  * (1.f - mx) + (l[i] + m + s) * mx) * og);
        r[i] = softLimit ((orr[i] * (1.f - mx) + (r[i] + m - s) * mx) * og);
    }
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
