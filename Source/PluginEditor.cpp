#include "PluginEditor.h"

using namespace vintage;

namespace
{
juce::String u8 (const char* t) { return juce::String (juce::CharPointer_UTF8 (t)); }

const juce::Colour lacquer   { 0xff3f4247 };   // Graphit (Mastering)
const juce::Colour silk      { 0xfff2eee4 };   // weißer Siebdruck
const juce::Colour silkDim   { 0xc8f2eee4 };
}

//==============================================================================
void RackLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& slider)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float knobR = R * 0.62f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Skala im Siebdruck
    const auto steps = juce::StringArray::fromTokens (slider.getProperties()["stepLabels"].toString(), "|", "");
    g.setColour (silk);
    if (steps.size() > 1)
    {
        for (int i = 0; i < steps.size(); ++i)
        {
            const float a = startAngle + (float) i / (float) (steps.size() - 1) * (endAngle - startAngle);
            const auto p1 = c.getPointOnCircumference (knobR + 4.0f, a), p2 = c.getPointOnCircumference (knobR + 11.0f, a);
            g.drawLine ({ p1, p2 }, 1.6f);
            if (steps[i].isNotEmpty())
            {
                g.setFont (labelFont (steps[i].length() > 3 ? 11.0f : 13.0f, 0.05f));
                const auto tp = c.getPointOnCircumference (knobR + (steps[i].length() > 3 ? 30.0f : 23.0f), a);
                g.drawText (steps[i], juce::Rectangle<float> (56.0f, 16.0f).withCentre (tp), juce::Justification::centred);
            }
        }
    }
    else
    {
        for (int i = 0; i <= 20; ++i)
        {
            const float a = startAngle + (float) i / 20.0f * (endAngle - startAngle);
            const bool major = i % 2 == 0;
            g.drawLine ({ c.getPointOnCircumference (knobR + 4.0f, a), c.getPointOnCircumference (knobR + (major ? 11.0f : 7.0f), a) }, major ? 1.6f : 1.0f);
            if (major)
            {
                g.setFont (labelFont (12.0f, 0.03f));
                g.drawText (juce::String (i / 2), juce::Rectangle<float> (24.0f, 14.0f).withCentre (c.getPointOnCircumference (knobR + 21.0f, a)), juce::Justification::centred);
            }
        }
    }

    // Schatten
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (c.translated (2.5f, 4.0f)));
    // Rock (geriffelt)
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3836), c.x, c.y - knobR, juce::Colour (0xff0c0b0a), c.x, c.y + knobR, false));
    g.fillEllipse (juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    for (int k = 0; k < 48; ++k)
    {
        const float a = (float) k / 48.0f * juce::MathConstants<float>::twoPi;
        g.drawLine ({ c.getPointOnCircumference (knobR * 0.86f, a), c.getPointOnCircumference (knobR - 0.5f, a) }, 1.2f);
    }
    // Kappe
    const float capR = knobR * 0.74f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4744), c.x - capR * 0.5f, c.y - capR, juce::Colour (0xff161514), c.x + capR * 0.4f, c.y + capR, false));
    g.fillEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c));
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillEllipse (juce::Rectangle<float> (capR * 1.1f, capR * 0.55f).withCentre (c.translated (-capR * 0.2f, -capR * 0.5f)));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c), 1.0f);
    // weiße Zeigerlinie über Kappe und Rock
    g.setColour (juce::Colour (0xfff5f1e6));
    g.drawLine ({ c.getPointOnCircumference (capR * 0.25f, angle), c.getPointOnCircumference (knobR - 1.5f, angle) }, 3.0f);
}

//==============================================================================
PultAudioProcessorEditor::PultAudioProcessorEditor (PultAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    // Reihenfolge = Bedienfeld: Tiefen (oben Frequenz, unten Anheben/Absenken), Höhen, Höhen-Absenkung, Ausgang
    addKnob ("lf",     "FREQUENZ (HZ)",   true, "20|30|60|100");
    addKnob ("lboost", "ANHEBEN",         true);
    addKnob ("latten", "ABSENKEN",        true);
    addKnob ("hf",     "FREQUENZ (KHZ)",  true, "3|4|5|8|10|12|16");
    addKnob ("bw",     "BANDBREITE",      true, "SCHMAL||||BREIT");
    addKnob ("hboost", "ANHEBEN",         true);
    addKnob ("haf",    "FREQUENZ (KHZ)",  true, "5|10|20");
    addKnob ("hatten", "ABSENKEN",        true);
    addKnob ("tube",   u8 ("R\xc3\x96HRE"), false);
    addKnob ("output", "PEGEL (DB)",      false, u8 ("\xe2\x88\x92" "12|\xe2\x88\x92" "6|0|+6|+12"));
    attachSet (0);

    setupToggle (onSwitch, "EIN", "AUS", lampRed);
    onAttachment = std::make_unique<ButtonAttachment> (proc.apvts, "on", onSwitch);

    setupToggle (modeSwitch, "M/S", "STEREO", lampAmber);
    modeSwitch.onClick = [this]
    {
        if (auto* prm = proc.apvts.getParameter ("mode"))
            prm->setValueNotifyingHost (modeSwitch.getToggleState() ? 1.0f : 0.0f);
    };

    setupToggle (editSwitch, "SEITE", "MITTE", lampAmber);
    editSwitch.setEnabled (false);
    editSwitch.setAlpha (0.4f);
    editSwitch.onClick = [this] { editSet = editSwitch.getToggleState() ? 1 : 0; attachSet (editSet); repaint(); };

    setupToggle (curveSwitch, "ZEIGEN", "AUS", lampGreen);
    curveVisible = (bool) proc.apvts.state.getProperty ("showCurve", false);
    curveSwitch.setToggleState (curveVisible, juce::dontSendNotification);
    curveSwitch.onClick = [this] { setCurveVisible (curveSwitch.getToggleState()); };

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    curve.setInterceptsMouseClicks (false, false);
    addChildComponent (curve);
    tubes.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (tubes);

    setSize (rackW, rackH + (curveVisible ? curveH : 0));
    curve.setVisible (curveVisible);
    startTimerHz (30);
}

PultAudioProcessorEditor::~PultAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

PultAudioProcessorEditor::Knob& PultAudioProcessorEditor::addKnob (const juce::String& baseID, const juce::String& name, bool perSet, const juce::String& steps)
{
    auto k = std::make_unique<Knob>();
    k->name = name; k->baseID = baseID; k->perSet = perSet;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k->slider.setMouseDragSensitivity (steps.isNotEmpty() && ! steps.contains ("SCHMAL") ? 140 : 240);
    if (steps.isNotEmpty())
        k->slider.getProperties().set ("stepLabels", steps);
    addAndMakeVisible (k->slider);
    if (! perSet)
    {
        k->attachment = std::make_unique<SliderAttachment> (proc.apvts, baseID, k->slider);
        if (auto* rp = proc.apvts.getParameter (baseID))
            k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    }
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void PultAudioProcessorEditor::attachSet (int set)
{
    for (auto& k : knobs)
        if (k->perSet)
        {
            k->attachment.reset();
            const auto id = PultAudioProcessor::pid (k->baseID.toRawUTF8(), set);
            k->attachment = std::make_unique<SliderAttachment> (proc.apvts, id, k->slider);
            if (auto* rp = proc.apvts.getParameter (id))
                k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
        }
    curve.editSet = set;
    curve.repaint();
}

void PultAudioProcessorEditor::setupToggle (juce::ToggleButton& b, const juce::String& onText, const juce::String& offText, juce::Colour lamp)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lamp.getARGB());
    b.setColour (juce::ToggleButton::textColourId, silk);
    addAndMakeVisible (b);
}

void PultAudioProcessorEditor::setCurveVisible (bool v)
{
    curveVisible = v;
    proc.apvts.state.setProperty ("showCurve", v, nullptr);
    curve.setVisible (v);
    setSize (rackW, rackH + (v ? curveH : 0));
}

void PultAudioProcessorEditor::timerCallback()
{
    // Röhren: Heizfaden glüht immer, mit Röhren-Regler und Signal etwas heller, leises Flackern
    {
        const float tubeAmt = proc.apvts.getRawParameterValue ("tube")->load() * 0.01f;
        const bool on = proc.apvts.getRawParameterValue ("on")->load() > 0.5f;
        const float lvl = juce::jlimit (0.0f, 1.0f, proc.vuOut.load() * 2.5f);
        smoothedLevel += (lvl - smoothedLevel) * (lvl > smoothedLevel ? 0.35f : 0.08f);
        bool changed = false;
        for (size_t i = 0; i < 2; ++i)
        {
            float target = 0.55f + 0.25f * tubeAmt + (on ? 0.22f : 0.0f) * smoothedLevel;
            target *= 0.97f + 0.05f * flicker.nextFloat();
            const float gv = tubes.glow[i] + (target - tubes.glow[i]) * 0.3f;
            changed |= std::abs (gv - tubes.glow[i]) > 0.002f;
            tubes.glow[i] = gv;
        }
        if (changed) tubes.repaint();
    }

    const bool ms = proc.isMidSide();
    modeSwitch.setToggleState (ms, juce::dontSendNotification);
    if (ms != shownMs)
    {
        shownMs = ms;
        editSwitch.setEnabled (ms);
        editSwitch.setAlpha (ms ? 1.0f : 0.4f);
        if (! ms && editSet != 0) { editSet = 0; editSwitch.setToggleState (false, juce::dontSendNotification); attachSet (0); }
        repaint();
    }

    const std::array<pult::Settings, 2> now { proc.readSettings (0), proc.readSettings (1) };
    if (! (now[0] == curve.shown[0]) || ! (now[1] == curve.shown[1]) || curve.ms != ms)
    {
        curve.shown = now; curve.ms = ms;
        curve.repaint();
    }

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

//==============================================================================
void PultAudioProcessorEditor::Curve::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4ead2), 0, b.getY(), juce::Colour (0xffe6d8b8), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 4.0f);

    const auto plot = b.reduced (34.0f, 12.0f).withTrimmedBottom (8.0f).withTrimmedRight (-20.0f);
    const double sr = proc.getSampleRateSafe() > 0 ? proc.getSampleRateSafe() : 48000.0;
    const double fLo = 20.0, fHi = 20000.0;
    auto xFor = [&] (double f) { return plot.getX() + plot.getWidth() * (float) (std::log (f / fLo) / std::log (fHi / fLo)); };
    const float range = 16.0f;
    auto yFor = [&] (double db) { return juce::jmap ((float) juce::jlimit (-range, range, (float) db), -range, range, plot.getBottom(), plot.getY()); };

    const juce::Colour gridC (0xffc9744f);
    g.setFont (lnf.labelFontRegular (9.5f));
    for (int fi : { 20, 30, 50, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 20000 })
    {
        const double f = fi;
        const float x = xFor (f);
        const bool major = fi == 100 || fi == 1000 || fi == 10000;
        g.setColour (gridC.withAlpha (major ? 0.35f : 0.15f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        if (fi == 20 || fi == 50 || fi == 100 || fi == 300 || fi == 1000 || fi == 3000 || fi == 10000 || fi == 20000)
        {
            g.setColour (gridC.withAlpha (0.85f));
            const auto label = fi >= 1000 ? juce::String (fi / 1000) + "k" : juce::String (fi);
            g.drawText (label, juce::Rectangle<float> (x - 20.0f, plot.getBottom() + 2.0f, 40.0f, 10.0f), juce::Justification::centred);
        }
    }
    for (int db = -15; db <= 15; db += 5)
    {
        const float y = yFor (db);
        g.setColour (gridC.withAlpha (db == 0 ? 0.55f : 0.18f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (gridC.withAlpha (0.85f));
        g.drawText ((db > 0 ? "+" : (db < 0 ? u8 ("\xe2\x88\x92") : juce::String())) + juce::String (std::abs (db)),
                    juce::Rectangle<float> (b.getX() + 2.0f, y - 6.0f, 28.0f, 12.0f), juce::Justification::centredRight);
    }

    auto drawSet = [&] (int set, juce::Colour c, float width)
    {
        const auto sec = pult::design (shown[(size_t) set], sr);
        juce::Path p;
        for (int i = 0; i <= 240; ++i)
        {
            const double f = fLo * std::pow (fHi / fLo, i / 240.0);
            const float x = xFor (f), y = yFor (pult::magnitudeDb (sec, std::min (f, 0.49 * sr), sr));
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (c.withAlpha (0.22f));
        g.strokePath (p, juce::PathStrokeType (width + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    const juce::Colour midC (0xff1a237e), sideC (0xff8e1c1c);
    if (ms)
    {
        drawSet (editSet == 0 ? 1 : 0, (editSet == 0 ? sideC : midC).withAlpha (0.45f), 1.3f);
        drawSet (editSet, editSet == 0 ? midC : sideC, 2.0f);
        g.setFont (lnf.labelFont (10.0f, 0.15f));
        g.setColour (midC);  g.drawText ("MITTE", juce::Rectangle<float> (plot.getRight() - 110.0f, plot.getY() + 2.0f, 50.0f, 12.0f), juce::Justification::centredRight);
        g.setColour (sideC); g.drawText ("SEITE", juce::Rectangle<float> (plot.getRight() - 56.0f, plot.getY() + 2.0f, 50.0f, 12.0f), juce::Justification::centredRight);
    }
    else
        drawSet (0, sideC, 2.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.18f), 0, b.getY(), juce::Colours::transparentBlack, 0, b.getY() + 8.0f, false));
    g.fillRect (b.withHeight (8.0f));
}

//==============================================================================
void PultAudioProcessorEditor::renderPanel()
{
    // Hammerschlaglack: Grundfarbe, weiche Flecken, feines Korn
    panelImage = juce::Image (juce::Image::ARGB, rackW, rackH, true);
    juce::Graphics g (panelImage);
    g.setGradientFill (juce::ColourGradient (lacquer.brighter (0.12f), 0, 0, lacquer.darker (0.18f), 0, (float) rackH, false));
    g.fillRect (0, 0, rackW, rackH);
    juce::Random rnd (11);
    for (int i = 0; i < 9000; ++i)
    {
        const float r = 1.5f + rnd.nextFloat() * 4.0f;
        const float x = rnd.nextFloat() * rackW, y = rnd.nextFloat() * rackH;
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.012f + rnd.nextFloat() * 0.02f));
        g.fillEllipse (x - r, y - r, 2 * r, 2 * r);
    }
    {
        juce::Image::BitmapData data (panelImage, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < rackH; ++y)
            for (int x = 0; x < rackW; ++x)
            {
                auto c = data.getPixelColour (x, y);
                const int n = rnd.nextInt (9) - 4;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, c.getRed() + n),
                                                         (juce::uint8) juce::jlimit (0, 255, c.getGreen() + n),
                                                         (juce::uint8) juce::jlimit (0, 255, c.getBlue() + n)));
            }
    }
    // Kanten: oben Licht, unten Schatten
    g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawHorizontalLine (0, 0.0f, (float) rackW);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.drawHorizontalLine (rackH - 1, 0.0f, (float) rackW);

    // Rack-Ohren mit Langlöchern
    for (float ex : { 0.0f, (float) rackW - 56.0f })
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawVerticalLine ((int) (ex < 1.0f ? 56.0f : ex), 0.0f, (float) rackH);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawVerticalLine ((int) (ex < 1.0f ? 57.0f : ex + 1.0f), 0.0f, (float) rackH);
        for (float sy : { 44.0f, (float) rackH - 44.0f })
        {
            const auto slot = juce::Rectangle<float> (ex + 14.0f, sy - 8.0f, 28.0f, 16.0f);
            g.setColour (juce::Colour (0xff141210));
            g.fillRoundedRectangle (slot, 8.0f);
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawRoundedRectangle (slot.translated (0.0f, 1.0f), 8.0f, 1.0f);
            VintageLookAndFeel::drawScrew (g, slot.getCentre().translated (ex < 1.0f ? 3.0f : -3.0f, 0.0f), 6.5f, sy * 0.01f + ex * 0.003f);
        }
    }

    // Abschnitte im Siebdruck
    const juce::String titles[] = { "TIEFEN", u8 ("H\xc3\x96HEN"), u8 ("H\xc3\x96HEN-ABSENKUNG"), u8 ("R\xc3\x96HRENSTUFE"), u8 ("R\xc3\x96HREN") };
    for (size_t i = 0; i < 5; ++i)
    {
        const auto r = sections[i].toFloat();
        g.setColour (silkDim);
        juce::Path box; box.addRoundedRectangle (r, 5.0f);
        g.strokePath (box, juce::PathStrokeType (1.2f));
        g.setFont (lnf.labelFont (13.0f, 0.3f));
        const float tw = (float) juce::GlyphArrangement::getStringWidthInt (lnf.labelFont (13.0f, 0.3f), titles[i]) + 20.0f;
        const auto tr = juce::Rectangle<float> (tw, 16.0f).withCentre ({ r.getCentreX(), r.getY() });
        g.setColour (lacquer.withAlpha (1.0f));
        g.fillRect (tr);
        g.setColour (silk);
        g.drawText (titles[i], tr, juce::Justification::centred);
    }

    // Beschriftungen unter den Knöpfen
    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        g.setColour (silk);
        g.setFont (lnf.labelFont (13.0f, 0.18f));
        g.drawText (k->name, juce::Rectangle<float> (sb.getX() - 30.0f, sb.getBottom() - 6.0f, sb.getWidth() + 60.0f, 18.0f), juce::Justification::centred);
    }

    // Röhren-Sichtfenster: Rahmen mit Schrauben und Typen-Gravur
    {
        const auto tb = tubeBezel.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (tb.translated (1.5f, 2.5f), 6.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb9bec3), 0, tb.getY(), juce::Colour (0xff5d6267), 0, tb.getBottom(), false));
        g.fillRoundedRectangle (tb, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawRoundedRectangle (tb.reduced (0.5f), 6.0f, 1.0f);
        for (auto pt : { tb.getTopLeft().translated (6.5f, 6.5f), tb.getTopRight().translated (-6.5f, 6.5f),
                         tb.getBottomLeft().translated (6.5f, -6.5f), tb.getBottomRight().translated (-6.5f, -6.5f) })
            VintageLookAndFeel::drawScrew (g, pt, 3.5f, pt.x * 0.01f + pt.y * 0.02f);
        const auto tw = tubes.getBounds().toFloat();
        g.setColour (silk);
        g.setFont (lnf.labelFont (11.0f, 0.2f));
        g.drawText ("12AX7", juce::Rectangle<float> (tw.getX() - 6.0f, tb.getBottom() + 6.0f, tw.getWidth() * 0.5f + 6.0f, 14.0f), juce::Justification::centred);
        g.drawText ("12AU7", juce::Rectangle<float> (tw.getCentreX(), tb.getBottom() + 6.0f, tw.getWidth() * 0.5f + 6.0f, 14.0f), juce::Justification::centred);
        g.setColour (silkDim);
        g.setFont (lnf.labelFont (9.5f, 0.2f));
        g.drawText (u8 ("VORSTUFE \xc2\xb7 TREIBER"), juce::Rectangle<float> (tb.getX() - 10.0f, tb.getBottom() + 24.0f, tb.getWidth() + 20.0f, 12.0f), juce::Justification::centred);
    }

    // Typenschild
    const auto np = nameplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (np.translated (1.5f, 2.5f), 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2522), 0, np.getY(), juce::Colour (0xff121010), 0, np.getBottom(), false));
    g.fillRoundedRectangle (np, 4.0f);
    g.setColour (gold.withAlpha (0.7f));
    g.drawRoundedRectangle (np.reduced (3.0f), 3.0f, 1.0f);
    g.setFont (lnf.scriptFont (34.0f));
    g.setColour (logo);
    g.drawText ("Klangbild", np.withTrimmedLeft (12.0f).withHeight (44.0f).translated (0.0f, 2.0f), juce::Justification::centredLeft);
    g.setColour (gold);
    g.setFont (lnf.labelFont (17.0f, 0.45f));
    g.drawText ("PULT", np.withTrimmedLeft (16.0f).withTrimmedTop (44.0f).withHeight (20.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (9.0f, 0.2f));
    g.drawText ("PROGRAMM-ENTZERRER", np.withTrimmedLeft (16.0f).withTrimmedTop (66.0f).withHeight (14.0f), juce::Justification::centredLeft);

    // Schalter-Beschriftungen
    const juce::String tl[] = { "EQ", "MODUS", "BEARBEITEN", "KURVE" };
    for (size_t i = 0; i < 4; ++i)
    {
        g.setColour (silk);
        g.setFont (lnf.labelFont (11.0f, 0.22f));
        g.drawText (tl[i], toggleLabels[i].toFloat(), juce::Justification::centredLeft);
    }
    g.setColour (silk);
    g.setFont (lnf.labelFont (11.0f, 0.22f));
    g.drawText ("PROGRAMM", presetBox.getBounds().toFloat().withY ((float) presetBox.getY() - 16.0f).withHeight (14.0f), juce::Justification::centredLeft);

    // Typ-Gravur unten rechts
    g.setColour (silkDim);
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText (u8 ("TYP KB-1A  \xc2\xb7  NR. 0612"), tagArea.toFloat(), juce::Justification::centredRight);
}

void PultAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (curveVisible)
    {
        g.drawImageAt (tolexImage, 0, rackH);
        const auto cb = curveBezel.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (cb.translated (2.0f, 3.0f), 8.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb9bec3), 0, cb.getY(), juce::Colour (0xff6d7277), 0, cb.getBottom(), false));
        g.fillRoundedRectangle (cb, 8.0f);
    }
    g.drawImageAt (panelImage, 0, 0);

    // Bearbeitungs-Hinweis im M/S-Modus
    if (shownMs)
    {
        g.setColour (lampAmber);
        g.setFont (lnf.labelFont (12.0f, 0.25f));
        const auto t = editSet == 0 ? juce::String ("EINSTELLUNG: MITTE") : juce::String ("EINSTELLUNG: SEITE");
        g.drawText (t, juce::Rectangle<float> ((float) sections[0].getX(), (float) rackH - 26.0f, 400.0f, 16.0f), juce::Justification::centredLeft);
    }
}

void PultAudioProcessorEditor::resized()
{
    panel = { 56, 0, rackW - 112, rackH };
    const int top = 30, bottom = rackH - 34;
    sections[0] = { 76, top, 310, bottom - top };
    sections[1] = { 398, top, 310, bottom - top };
    sections[2] = { 720, top, 150, bottom - top };
    sections[3] = { 882, top, 150, bottom - top };
    sections[4] = { 1044, top, 150, bottom - top };
    tubeBezel = sections[4].reduced (12).withTrimmedTop (14).withTrimmedBottom (46);
    tubes.setBounds (tubeBezel.reduced (12));

    const int topKnob = 150, botKnob = 140;
    auto placeTop = [&] (int idx, int cx) { knobs[(size_t) idx]->slider.setBounds (cx - topKnob / 2, top + 20, topKnob, topKnob); };
    auto placeBot = [&] (int idx, int cx) { knobs[(size_t) idx]->slider.setBounds (cx - botKnob / 2, top + 210, botKnob, botKnob); };
    placeTop (0, sections[0].getCentreX());
    placeBot (1, sections[0].getX() + 80); placeBot (2, sections[0].getRight() - 80);
    placeTop (3, sections[1].getCentreX());
    placeBot (4, sections[1].getX() + 80); placeBot (5, sections[1].getRight() - 80);
    placeTop (6, sections[2].getCentreX()); placeBot (7, sections[2].getCentreX());
    placeTop (8, sections[3].getCentreX()); placeBot (9, sections[3].getCentreX());

    // rechte Spalte: Typenschild, Programm, Schalter
    const int colX = 1206, colW = rackW - 56 - 14 - colX;
    nameplate = { colX, top - 8, colW, 88 };
    presetBox.setBounds (colX, top + 100, colW, 28);
    juce::ToggleButton* tg[] = { &onSwitch, &modeSwitch, &editSwitch, &curveSwitch };
    for (int i = 0; i < 4; ++i)
    {
        const int y = top + 140 + i * 56;
        toggleLabels[(size_t) i] = { colX, y, colW, 14 };
        tg[i]->setBounds (colX - 4, y + 14, 140, 40);
    }
    tagArea = { colX - 40, rackH - 28, colW + 40, 16 };

    curveBezel = { 56, rackH + 14, rackW - 112, curveH - 28 };
    curve.setBounds (curveBezel.reduced (8));

    tolexImage = juce::Image (juce::Image::RGB, rackW, curveH, false);
    {
        juce::Image::BitmapData data (tolexImage, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (7);
        for (int y = 0; y < curveH; ++y)
            for (int x = 0; x < rackW; ++x)
            {
                const int n = rnd.nextInt (11) - 5;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) (24 + n), (juce::uint8) (22 + n), (juce::uint8) (21 + n)));
            }
    }
    renderPanel();
}

//==============================================================================
void PultAudioProcessorEditor::TubeWindow::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // dunkler Innenraum hinter dem Fenster
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1714), 0, b.getY(), juce::Colour (0xff070606), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 3.0f);

    // Chassis-Blech, auf dem die Fassungen sitzen
    const float deckY = b.getY() + b.getHeight() * 0.86f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4c50), 0, deckY, juce::Colour (0xff1d1e20), 0, b.getBottom(), false));
    g.fillRect (b.withTop (deckY));

    const float glassW = b.getWidth() * 0.36f;
    const float glassBottom = deckY - glassW * 0.36f;
    const float glassH = juce::jmin (glassW * 4.6f, glassBottom - b.getY() - 22.0f);
    const float cx[] = { b.getX() + b.getWidth() * 0.27f, b.getX() + b.getWidth() * 0.73f };
    // Gesamtschein im Innenraum
    {
        const float gm = 0.5f * (glow[0] + glow[1]);
        const auto c = juce::Point<float> (b.getCentreX(), glassBottom - glassH * 0.4f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff7a1e).withAlpha (0.28f * gm), c.x, c.y,
                                                 juce::Colour (0x00ff7a1e), c.x + b.getWidth() * 0.9f, c.y, true));
        g.fillRect (b);
    }
    for (int i = 0; i < 2; ++i)
    {
        const float gl = glow[(size_t) i];
        // warmer Schein aufs Chassis
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff7a1e).withAlpha (0.45f * gl), cx[i], deckY,
                                                 juce::Colour (0x00ff7a1e), cx[i] + glassW * 1.3f, deckY, true));
        g.fillEllipse (cx[i] - glassW * 1.3f, deckY - glassW * 0.35f, glassW * 2.6f, glassW * 0.8f);
        drawTube (g, { cx[i] - glassW * 0.5f, glassBottom - glassH, glassW, glassH }, gl, i == 0);
    }

    // Spiegelung auf der Fensterscheibe
    juce::Path refl;
    refl.startNewSubPath (b.getX(), b.getY() + b.getHeight() * 0.35f);
    refl.lineTo (b.getX() + b.getWidth() * 0.55f, b.getY());
    refl.lineTo (b.getX() + b.getWidth() * 0.8f, b.getY());
    refl.lineTo (b.getX(), b.getY() + b.getHeight() * 0.62f);
    refl.closeSubPath();
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.fillPath (refl);

    // Innenschatten der Fensterkante
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 2.0f);
}

void PultAudioProcessorEditor::TubeWindow::drawTube (juce::Graphics& g, juce::Rectangle<float> r, float gl, bool twinPlates)
{
    const juce::Colour hot (0xffff8a2a), core (0xffffe0a0);
    const float w = r.getWidth();

    // Glaskolben-Umriss: oben Kuppel, unten leicht gerundet
    juce::Path glass;
    glass.addRoundedRectangle (r.getX(), r.getY(), w, r.getHeight(), w * 0.5f, w * 0.5f, true, true, false, false);

    // Hof hinter dem Glas
    const auto plateC = juce::Point<float> (r.getCentreX(), r.getY() + r.getHeight() * 0.55f);
    g.setGradientFill (juce::ColourGradient (hot.withAlpha (0.6f * gl), plateC.x, plateC.y,
                                             hot.withAlpha (0.0f), plateC.x + w * 1.6f, plateC.y, true));
    g.fillEllipse (juce::Rectangle<float> (w * 3.2f, r.getHeight() * 1.0f).withCentre (plateC));

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (glass);

        // Getter-Spiegel oben in der Kuppel
        const float getterH = r.getHeight() * 0.17f;
        juce::ColourGradient gg (juce::Colour (0xffa6acb4), r.getX(), r.getY(), juce::Colour (0xff1e2024), r.getRight(), r.getY() + getterH, false);
        gg.addColour (0.55, juce::Colour (0xff4a4e55));
        g.setGradientFill (gg);
        g.fillRect (r.withHeight (getterH));
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, 0, r.getY() + getterH * 0.5f,
                                                 juce::Colours::black.withAlpha (0.0f), 0, r.getY() + getterH, false));

        const float topMica = r.getY() + r.getHeight() * 0.25f, botMica = r.getY() + r.getHeight() * 0.86f;

        // Stützstäbe
        g.setColour (juce::Colour (0xff8d8f93).withAlpha (0.85f));
        for (float fx : { 0.2f, 0.8f })
            g.fillRect (r.getX() + w * fx - 0.6f, topMica - 4.0f, 1.2f, r.getBottom() - topMica + 4.0f);

        // Anodenbleche: zwei Systeme übereinander
        const float plateW = w * 0.58f, gap = r.getHeight() * 0.04f;
        const float stackTop = topMica + r.getHeight() * 0.03f, stackBot = botMica - r.getHeight() * 0.03f;
        const float unitH = (stackBot - stackTop - gap) * 0.5f;
        const float plateX = r.getCentreX() - plateW * 0.5f;
        auto drawPlate = [&] (float y, float h)
        {
            const juce::Rectangle<float> pr (plateX, y, plateW, h);
            juce::ColourGradient cg (juce::Colour (0xff17181a), pr.getX(), 0, juce::Colour (0xff17181a), pr.getRight(), 0, false);
            cg.addColour (0.45, juce::Colour (0xff4e5055));
            g.setGradientFill (cg);
            g.fillRect (pr);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            for (float rx : { 0.2f, 0.5f, 0.8f })
                g.fillRect (pr.getX() + pr.getWidth() * rx - 0.5f, pr.getY(), 1.0f, pr.getHeight());
            g.setGradientFill (juce::ColourGradient (hot.withAlpha (0.42f * gl), pr.getCentreX(), pr.getBottom(),
                                                     hot.withAlpha (0.12f * gl), pr.getCentreX(), pr.getY(), false));   // warmer Widerschein
            g.fillRect (pr);
            // Heizfaden schimmert durch die Schlitze an den Blechkanten
            g.setColour (hot.withAlpha (0.55f * gl));
            g.fillRect (pr.getX() - 1.5f, pr.getY() + 3.0f, 1.5f, pr.getHeight() - 6.0f);
            g.fillRect (pr.getRight(), pr.getY() + 3.0f, 1.5f, pr.getHeight() - 6.0f);
        };
        const float shrink = twinPlates ? 1.0f : 0.85f;
        drawPlate (stackTop + unitH * (1.0f - shrink), unitH * shrink);
        drawPlate (stackTop + unitH + gap, unitH * shrink);

        // glühender Heizfaden: in der Lücke und an den Enden der Bleche sichtbar
        auto glowLine = [&] (float y, float len)
        {
            for (int k = 4; k >= 1; --k)
            {
                g.setColour (hot.withAlpha (juce::jmin (1.0f, 0.2f * gl * (float) (5 - k))));
                g.fillEllipse (juce::Rectangle<float> (len + (float) k * 10.0f, 3.0f + (float) k * 6.0f).withCentre ({ r.getCentreX(), y }));
            }
            g.setColour (core.withAlpha (juce::jlimit (0.0f, 1.0f, 0.5f + 0.55f * gl)));
            g.fillRoundedRectangle (juce::Rectangle<float> (len, 2.0f).withCentre ({ r.getCentreX(), y }), 1.0f);
        };
        glowLine (stackTop + unitH + gap * 0.5f, plateW * 0.45f);
        glowLine (stackTop + unitH * (1.0f - shrink) - 2.5f, plateW * 0.25f);
        glowLine (stackBot + 2.5f, plateW * 0.25f);

        // Glimmerscheiben oben und unten
        g.setColour (juce::Colour (0xffd8d4cc).withAlpha (0.55f));
        for (float my : { topMica, botMica })
            g.fillRoundedRectangle (juce::Rectangle<float> (w * 0.9f, 3.0f).withCentre ({ r.getCentreX(), my }), 1.5f);

        // Glühen legt sich übers Ganze
        g.setGradientFill (juce::ColourGradient (hot.withAlpha (0.22f * gl), plateC.x, plateC.y, hot.withAlpha (0.0f), plateC.x + w * 0.75f, plateC.y, true));
        g.fillRect (r);

        // Glas: Kanten heller, Mitte klar
        juce::ColourGradient eg (juce::Colours::white.withAlpha (0.22f), r.getX(), 0, juce::Colours::white.withAlpha (0.16f), r.getRight(), 0, false);
        eg.addColour (0.18, juce::Colours::white.withAlpha (0.02f));
        eg.addColour (0.82, juce::Colours::white.withAlpha (0.02f));
        g.setGradientFill (eg);
        g.fillRect (r);
    }

    // Kontur, Glanzstreifen, Abschmelzspitze
    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.strokePath (glass, juce::PathStrokeType (1.0f));
    g.setColour (juce::Colours::white.withAlpha (0.38f));
    g.fillRoundedRectangle (r.getX() + w * 0.16f, r.getY() + w * 0.45f, w * 0.07f, r.getHeight() * 0.6f, w * 0.035f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.fillRoundedRectangle (r.getCentreX() - 2.0f, r.getY() - 5.0f, 4.0f, 7.0f, 2.0f);

    // Sockel aus Pressstoff
    const juce::Rectangle<float> base (r.getX() - w * 0.04f, r.getBottom(), w * 1.08f, w * 0.36f);
    juce::ColourGradient bg (juce::Colour (0xff121212), base.getX(), 0, juce::Colour (0xff101010), base.getRight(), 0, false);
    bg.addColour (0.35, juce::Colour (0xff3a3a3a));
    g.setGradientFill (bg);
    g.fillRoundedRectangle (base, 2.5f);
    g.setColour (hot.withAlpha (0.3f * gl));
    g.drawHorizontalLine ((int) base.getY(), base.getX() + 2.0f, base.getRight() - 2.0f);
}
