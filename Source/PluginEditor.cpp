#include "PluginEditor.h"

using juce::Colour;
using juce::Graphics;

namespace
{
    constexpr int W = 752, H = 400, topH = 76;
    const Colour woodLight (0xffe9d3ae), woodMid (0xffd9bc8e), woodDark (0xffb98b52);
    const Colour inkSoft (0xff8a7c6a), inkWood (0xff6b5a44);

    const char* ids[]   = { "compAmt", "compSpd", "airAmt", "airTone", "dlyMix", "dlyTime", "revMix", "revSize", "output" };
    const char* names[] = { "Amount", "Speed", "Air", "Tone", "Mix", "Time", "Mix", "Size", "Output" };
    const char* titles[] = { "DYNAMICS", "AIR", "DELAY", "REVERB" };
}

//==============================================================================
void MenaceLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h,
                                          float pos, float startAngle, float endAngle,
                                          juce::Slider&)
{
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float d  = juce::jmin (area.getWidth(), area.getHeight());
    const float cx = area.getCentreX(), cy = area.getCentreY();
    const float r  = d * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // trilho do valor
    const float rt = r - 1.5f;
    juce::Path track, value;
    track.addCentredArc (cx, cy, rt, rt, 0.f, startAngle, endAngle, true);
    value.addCentredArc (cx, cy, rt, rt, 0.f, startAngle, angle, true);
    g.setColour (Colour (0xff8a7c6a).withAlpha (0.18f));
    g.strokePath (track, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (woodDark);
    g.strokePath (value, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // corpo de porcelana
    const float kr = r - 7.f;
    auto body = juce::Rectangle<float> (cx - kr, cy - kr, kr * 2.f, kr * 2.f);
    juce::Path bp; bp.addEllipse (body);
    juce::DropShadow (juce::Colours::black.withAlpha (0.28f), 10, { 0, 5 }).drawForPath (g, bp);

    g.setGradientFill (juce::ColourGradient (Colour (0xffffffff), cx - kr * 0.5f, cy - kr * 0.8f,
                                             Colour (0xffe9e4dc), cx + kr * 0.5f, cy + kr, false));
    g.fillEllipse (body);
    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.drawEllipse (body.reduced (0.75f), 1.5f);

    // concavidade central
    auto inner = body.reduced (kr * 0.30f);
    g.setGradientFill (juce::ColourGradient (Colour (0xffe6e1d8), cx, inner.getY(),
                                             Colour (0xfffdfcfa), cx, inner.getBottom(), false));
    g.fillEllipse (inner);

    // ponteiro de madeira
    juce::Path ptr;
    ptr.addRoundedRectangle (-2.5f, -kr * 0.88f, 5.f, kr * 0.36f, 2.f);
    g.setColour (woodDark);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

//==============================================================================
MenaceAudioEditor::MenaceAudioEditor (MenaceAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < numKnobs; ++i)
    {
        auto& s = sliders[(size_t) i];
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        s.setPopupDisplayEnabled (true, true, this);
        s.setLookAndFeel (&laf);
        addAndMakeVisible (s);
        attachments[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, ids[i], s);
    }
    setSize (W, H);
}

MenaceAudioEditor::~MenaceAudioEditor()
{
    for (auto& s : sliders) s.setLookAndFeel (nullptr);
}

void MenaceAudioEditor::resized()
{
    for (int i = 0; i < 4; ++i)
    {
        cards[(size_t) i] = { 24 + i * 180, 96, 164, 272 };
        for (int j = 0; j < 2; ++j)
        {
            knobRects[(size_t) (i * 2 + j)] = { cards[(size_t) i].getX() + 40,
                                                cards[(size_t) i].getY() + 44 + j * 112, 84, 84 };
        }
    }
    knobRects[8] = { W - 24 - 60, 8, 60, 60 };
    for (int i = 0; i < numKnobs; ++i) sliders[(size_t) i].setBounds (knobRects[(size_t) i]);
}

void MenaceAudioEditor::paint (Graphics& g)
{
    const auto w = (float) W;

    // fundo porcelana
    g.setGradientFill (juce::ColourGradient (Colour (0xfffbf9f6), 0.f, 0.f, Colour (0xffe8e3db), 0.f, (float) H, false));
    g.fillAll();

    // manchas suaves atrás do vidro, para a transparência ter o que mostrar
    auto blob = [&] (float cx, float cy, float rad, Colour c)
    {
        g.setGradientFill (juce::ColourGradient (c, cx, cy, c.withAlpha (0.f), cx + rad, cy, true));
        g.fillEllipse (cx - rad, cy - rad, rad * 2.f, rad * 2.f);
    };
    blob (120.f, 300.f, 170.f, Colour (0xffd9bf94).withAlpha (0.35f));
    blob (640.f, 200.f, 200.f, Colour (0xffcfd6dc).withAlpha (0.45f));
    blob (380.f, 380.f, 160.f, Colour (0xffe4cfaa).withAlpha (0.30f));

    // faixa de madeira clara com veios
    juce::Rectangle<float> wood (0.f, 0.f, w, (float) topH);
    juce::Path wp; wp.addRectangle (wood);
    juce::DropShadow (juce::Colours::black.withAlpha (0.25f), 14, { 0, 4 }).drawForPath (g, wp);
    g.setGradientFill (juce::ColourGradient (woodLight, 0.f, 0.f, woodMid, 0.f, (float) topH, false));
    g.fillRect (wood);

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (wood.toNearestInt());
        juce::Random rng (11);
        for (int i = 0; i < 70; ++i)
        {
            const float y = rng.nextFloat() * (float) topH;
            const float x0 = rng.nextFloat() * w * 0.5f;
            const float len = 120.f + rng.nextFloat() * 400.f;
            juce::Path p;
            p.startNewSubPath (x0, y);
            p.cubicTo (x0 + len * 0.3f, y + rng.nextFloat() * 4.f - 2.f,
                       x0 + len * 0.6f, y + rng.nextFloat() * 4.f - 2.f,
                       x0 + len,        y + rng.nextFloat() * 3.f - 1.5f);
            g.setColour (Colour (0xff9c7440).withAlpha (0.05f + rng.nextFloat() * 0.08f));
            g.strokePath (p, juce::PathStrokeType (0.6f + rng.nextFloat() * 0.8f));
        }
    }

    // título
    g.setColour (inkWood);
    g.setFont (juce::Font (juce::FontOptions (22.f).withStyle ("Bold").withKerningFactor (0.18f)));
    g.drawText ("MENACE AUDIO", 28, 12, 320, 30, juce::Justification::centredLeft);
    g.setColour (inkWood.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (11.f).withKerningFactor (0.3f)));
    g.drawText ("VOCAL CHAIN", 29, 42, 200, 16, juce::Justification::centredLeft);

    g.setFont (juce::Font (juce::FontOptions (10.f).withStyle ("Bold").withKerningFactor (0.25f)));
    g.drawText ("OUTPUT", W - 24 - 60 - 90, 28, 84, 16, juce::Justification::centredRight);

    // cartões de vidro branco
    for (int i = 0; i < 4; ++i)
    {
        auto c = cards[(size_t) i].toFloat();
        juce::Path cp; cp.addRoundedRectangle (c, 18.f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.16f), 18, { 0, 8 }).drawForPath (g, cp);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.82f), c.getX(), c.getY(),
                                                 juce::Colours::white.withAlpha (0.38f), c.getX(), c.getBottom(), false));
        g.fillPath (cp);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.strokePath (cp, juce::PathStrokeType (1.2f));
        g.setColour (juce::Colours::white);
        g.drawLine (c.getX() + 14.f, c.getY() + 1.5f, c.getRight() - 14.f, c.getY() + 1.5f, 1.2f);

        g.setColour (inkSoft);
        g.setFont (juce::Font (juce::FontOptions (11.f).withStyle ("Bold").withKerningFactor (0.25f)));
        g.drawText (titles[i], cards[(size_t) i].withHeight (34).withTrimmedTop (8), juce::Justification::centred);
    }

    // nomes dos knobs
    g.setColour (inkSoft);
    g.setFont (juce::Font (juce::FontOptions (11.f).withKerningFactor (0.08f)));
    for (int i = 0; i < 8; ++i)
    {
        auto k = knobRects[(size_t) i];
        g.drawText (names[i], k.getX() - 10, k.getBottom() - 2, k.getWidth() + 20, 18, juce::Justification::centred);
    }
}
