#include "PluginEditor.h"

using juce::Colour;
using juce::Graphics;

namespace
{
    constexpr int W = 912, H = 690, topH = 84, botH = 22;

    const Colour woodLight (0xffeed9b6), woodMid (0xffdcc093), woodDark (0xffb88a50);
    const Colour inkSoft (0xff8d8071), inkWood (0xff5f4e3a);

    const char* labels[MA::numParams] = {
        "GAIN", "HPF", "TAME", "GATE",
        "DE-ESS", "COMP", "SPEED", "WARMTH",
        "AIR", "FREQ", "DRIVE", "SPARK",
        "MIX", "TIME", "FEEDBACK", "DARK",
        "MIX", "SIZE", "PRE", "DAMP",
        "WIDTH", "MIX", "OUT" };
    const char* titles[6] = { "INPUT", "DYNAMICS", "AIR", "DELAY", "REVERB", "MASTER" };

    juce::Font ui (float size, bool bold = false, float kerning = 0.f)
    {
        return juce::Font (juce::FontOptions (bold ? "Segoe UI Semibold" : "Segoe UI", size, juce::Font::plain)
                               .withKerningFactor (kerning));
    }

    void drawWood (Graphics& g, juce::Rectangle<float> r, int seed)
    {
        g.setGradientFill (juce::ColourGradient (woodLight, 0.f, r.getY(), woodMid, 0.f, r.getBottom(), false));
        g.fillRect (r);
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.toNearestInt());
        juce::Random rng (seed);
        for (int i = 0; i < 90; ++i)
        {
            const float y = r.getY() + rng.nextFloat() * r.getHeight();
            const float x0 = rng.nextFloat() * r.getWidth() * 0.6f;
            const float len = 100.f + rng.nextFloat() * 420.f;
            juce::Path p;
            p.startNewSubPath (x0, y);
            p.cubicTo (x0 + len * 0.3f, y + rng.nextFloat() * 4.f - 2.f,
                       x0 + len * 0.6f, y + rng.nextFloat() * 4.f - 2.f,
                       x0 + len,        y + rng.nextFloat() * 3.f - 1.5f);
            g.setColour (Colour (0xff946b3a).withAlpha (0.05f + rng.nextFloat() * 0.10f));
            g.strokePath (p, juce::PathStrokeType (0.5f + rng.nextFloat() * 0.9f));
        }
        // brilho de verniz no topo
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), 0.f, r.getY(),
                                                 juce::Colours::white.withAlpha (0.f), 0.f, r.getY() + r.getHeight() * 0.5f, false));
        g.fillRect (r);
    }
}

//==============================================================================
MenaceLookAndFeel::MenaceLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::white.withAlpha (0.55f));
    setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.9f));
    setColour (juce::ComboBox::textColourId, inkWood);
    setColour (juce::ComboBox::arrowColourId, inkWood);
    setColour (juce::PopupMenu::backgroundColourId, Colour (0xfffbf9f6));
    setColour (juce::PopupMenu::textColourId, inkWood);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, woodLight);
    setColour (juce::PopupMenu::highlightedTextColourId, inkWood);
}

juce::Font MenaceLookAndFeel::getComboBoxFont (juce::ComboBox&) { return ui (13.f, true, 0.08f); }
juce::Font MenaceLookAndFeel::getPopupMenuFont() { return ui (13.f, false, 0.05f); }

void MenaceLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h,
                                          float pos, float startAngle, float endAngle,
                                          juce::Slider& s)
{
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float cx = area.getCentreX(), cy = area.getCentreY();
    const float R  = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // pontos das travas: um por posição possível; acesos até o valor atual
    const double range = s.getMaximum() - s.getMinimum();
    const int steps = s.getInterval() > 0.0 ? juce::jlimit (2, 40, (int) std::round (range / s.getInterval())) : 20;
    const float tickR = R - 2.f;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float) i / (float) steps;
        const float a = startAngle + t * (endAngle - startAngle);
        const float px = cx + std::sin (a) * tickR;
        const float py = cy - std::cos (a) * tickR;
        const bool lit = t <= pos + 0.001f;
        const float rad = (i == 0 || i == steps) ? 2.0f : 1.5f;
        g.setColour (lit ? woodDark : Colour (0xff8d8071).withAlpha (0.35f));
        g.fillEllipse (px - rad, py - rad, rad * 2.f, rad * 2.f);
    }

    // base de madeira clara
    const float pr = R - 8.f;
    juce::Path basePath; basePath.addEllipse (cx - pr, cy - pr, pr * 2.f, pr * 2.f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.22f), 8, { 0, 4 }).drawForPath (g, basePath);
    g.setGradientFill (juce::ColourGradient (woodLight, cx - pr * 0.6f, cy - pr, woodMid, cx + pr * 0.6f, cy + pr, false));
    g.fillPath (basePath);
    g.setColour (Colour (0xff946b3a).withAlpha (0.25f));
    g.strokePath (basePath, juce::PathStrokeType (0.8f));

    // corpo de porcelana
    const float kr = pr - 4.5f;
    juce::Path kp; kp.addEllipse (cx - kr, cy - kr, kr * 2.f, kr * 2.f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.30f), 7, { 0, 3 }).drawForPath (g, kp);
    g.setGradientFill (juce::ColourGradient (Colour (0xffffffff), cx - kr * 0.5f, cy - kr * 0.85f,
                                             Colour (0xffe6e0d6), cx + kr * 0.6f, cy + kr, false));
    g.fillPath (kp);

    // serrilha (knurl) girando com o knob
    for (int i = 0; i < 48; ++i)
    {
        const float a = angle + (float) i * juce::MathConstants<float>::twoPi / 48.f;
        const float s0 = std::sin (a), c0 = std::cos (a);
        g.setColour (Colour (0xff6b5a44).withAlpha (0.20f));
        g.drawLine (cx + s0 * kr * 0.88f, cy - c0 * kr * 0.88f, cx + s0 * kr * 0.985f, cy - c0 * kr * 0.985f, 0.9f);
        const float a2 = a + 0.035f;
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawLine (cx + std::sin (a2) * kr * 0.88f, cy - std::cos (a2) * kr * 0.88f,
                    cx + std::sin (a2) * kr * 0.985f, cy - std::cos (a2) * kr * 0.985f, 0.7f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.drawEllipse (cx - kr + 0.5f, cy - kr + 0.5f, kr * 2.f - 1.f, kr * 2.f - 1.f, 1.2f);

    // concavidade central com o valor
    const float ir = kr * 0.50f;
    g.setGradientFill (juce::ColourGradient (Colour (0xffe3ddd2), cx, cy - ir, Colour (0xfffefdfb), cx, cy + ir, false));
    g.fillEllipse (cx - ir, cy - ir, ir * 2.f, ir * 2.f);
    g.setColour (Colour (0xff6b5a44).withAlpha (0.12f));
    g.drawEllipse (cx - ir, cy - ir, ir * 2.f, ir * 2.f, 0.8f);

    const auto txt = s.getTextFromValue (s.getValue());
    g.setColour (inkWood);
    g.setFont (ui (txt.length() > 3 ? 8.5f : 10.5f, true));
    g.drawText (txt, juce::Rectangle<float> (cx - ir, cy - ir, ir * 2.f, ir * 2.f), juce::Justification::centred, false);

    // ponteiro de madeira
    juce::Path ptr;
    ptr.addRoundedRectangle (-2.2f, -kr * 0.86f, 4.4f, kr * 0.30f, 2.f);
    g.setColour (woodDark);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

//==============================================================================
MenaceAudioEditor::MenaceAudioEditor (MenaceAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < MA::numParams; ++i)
    {
        auto& s = sliders[(size_t) i];
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        s.setMouseDragSensitivity (200);
        s.setPopupDisplayEnabled (true, true, this);
        s.setLookAndFeel (&laf);
        addAndMakeVisible (s);
        attachments[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, MA::paramIds[i], s);
    }

    for (int i = 0; i < MA::numPresets; ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setLookAndFeel (&laf);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    setSize (W, H);
}

MenaceAudioEditor::~MenaceAudioEditor()
{
    for (auto& s : sliders) s.setLookAndFeel (nullptr);
    presetBox.setLookAndFeel (nullptr);
}

void MenaceAudioEditor::resized()
{
    for (int i = 0; i < 6; ++i)
        cards[(size_t) i] = { 24 + (i % 3) * 294, 106 + (i / 3) * 280, 276, 262 };

    for (int k = 0; k < MA::numParams; ++k)
    {
        const int ci = k / 4, j = k % 4;
        const auto& c = cards[(size_t) ci];
        int col = j % 2, row = j / 2;
        int cx = c.getX() + (col == 0 ? 74 : 202);
        if (ci == 5 && j == 2) cx = c.getCentreX();    // OUT centralizado
        knobRects[(size_t) k] = { cx - 40, c.getY() + 42 + row * 110, 80, 80 };
        sliders[(size_t) k].setBounds (knobRects[(size_t) k]);
    }
    presetBox.setBounds (W - 24 - 220, 28, 220, 32);
}

void MenaceAudioEditor::paint (Graphics& g)
{
    const auto w = (float) W;

    // fundo porcelana
    g.setGradientFill (juce::ColourGradient (Colour (0xfffcfaf7), 0.f, 0.f, Colour (0xffe6e0d7), 0.f, (float) H, false));
    g.fillAll();

    auto blob = [&] (float cx, float cy, float rad, Colour c)
    {
        g.setGradientFill (juce::ColourGradient (c, cx, cy, c.withAlpha (0.f), cx + rad, cy, true));
        g.fillEllipse (cx - rad, cy - rad, rad * 2.f, rad * 2.f);
    };
    blob (140.f, 330.f, 230.f, Colour (0xffd9bf94).withAlpha (0.40f));
    blob (780.f, 220.f, 260.f, Colour (0xffc9d2da).withAlpha (0.50f));
    blob (450.f, 600.f, 240.f, Colour (0xffe4cfaa).withAlpha (0.38f));
    blob (470.f, 150.f, 200.f, Colour (0xfff2e6d2).withAlpha (0.60f));

    // faixa de madeira (topo) e base
    juce::Rectangle<float> top (0.f, 0.f, w, (float) topH), bot (0.f, (float) (H - botH), w, (float) botH);
    juce::Path tp; tp.addRectangle (top);
    juce::DropShadow (juce::Colours::black.withAlpha (0.28f), 16, { 0, 5 }).drawForPath (g, tp);
    drawWood (g, top, 11);
    juce::Path bp; bp.addRectangle (bot);
    juce::DropShadow (juce::Colours::black.withAlpha (0.18f), 12, { 0, -3 }).drawForPath (g, bp);
    drawWood (g, bot, 29);

    // título
    g.setColour (inkWood);
    g.setFont (ui (26.f, true, 0.30f));
    g.drawText ("MENACE AUDIO", 30, 12, 380, 36, juce::Justification::centredLeft);
    g.setColour (inkWood.withAlpha (0.72f));
    g.setFont (ui (10.5f, false, 0.45f));
    g.drawText ("TRAP VOCAL CHAIN", 32, 48, 260, 16, juce::Justification::centredLeft);
    g.setFont (ui (9.5f, true, 0.35f));
    g.drawText ("PRESET", W - 24 - 220, 12, 100, 14, juce::Justification::centredLeft);

    // cartões de vidro branco
    for (int i = 0; i < 6; ++i)
    {
        auto c = cards[(size_t) i].toFloat();
        juce::Path cp; cp.addRoundedRectangle (c, 20.f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.17f), 22, { 0, 9 }).drawForPath (g, cp);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.86f), c.getX(), c.getY(),
                                                 juce::Colours::white.withAlpha (0.40f), c.getX(), c.getBottom(), false));
        g.fillPath (cp);

        {   // reflexo diagonal do vidro
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (cp);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.55f), c.getX(), c.getY(),
                                                     juce::Colours::white.withAlpha (0.f), c.getX() + c.getWidth() * 0.55f,
                                                     c.getY() + c.getHeight() * 0.45f, false));
            g.fillPath (cp);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.f), 0.f, c.getBottom() - 50.f,
                                                     juce::Colours::black.withAlpha (0.045f), 0.f, c.getBottom(), false));
            g.fillPath (cp);
        }

        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.strokePath (cp, juce::PathStrokeType (1.3f));
        g.setColour (Colour (0xff6b5a44).withAlpha (0.08f));
        g.strokePath (cp, juce::PathStrokeType (0.6f));

        // título da seção com fio de madeira
        g.setColour (inkSoft);
        g.setFont (ui (11.f, true, 0.40f));
        g.drawText (titles[i], cards[(size_t) i].withHeight (36).withTrimmedTop (10), juce::Justification::centred);
        g.setColour (woodMid.withAlpha (0.8f));
        g.fillRoundedRectangle (c.getCentreX() - 14.f, c.getY() + 34.f, 28.f, 2.f, 1.f);
    }

    // nomes dos knobs
    g.setColour (inkSoft);
    g.setFont (ui (10.f, true, 0.22f));
    for (int k = 0; k < MA::numParams; ++k)
    {
        auto r = knobRects[(size_t) k];
        g.drawText (labels[k], r.getX() - 14, r.getBottom() - 2, r.getWidth() + 28, 16, juce::Justification::centred);
    }
}
