#include "PluginEditor.h"

using namespace vintage;

namespace
{
juce::String u8 (const char* t) { return juce::String (juce::CharPointer_UTF8 (t)); }
}

PultAudioProcessorEditor::PultAudioProcessorEditor (PultAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    // Reihenfolge = Position auf der Frontplatte
    addKnob ("lf",     "FREQUENZ (HZ)",  true, "20|30|60|100");
    addKnob ("lboost", "ANHEBEN",        true);
    addKnob ("latten", "ABSENKEN",       true);
    addKnob ("hf",     "FREQUENZ (KHZ)", true, "3|4|5|8|10|12|16");
    addKnob ("bw",     "BANDBREITE",     true, "SCHMAL||||BREIT");
    addKnob ("hboost", "ANHEBEN",        true);
    addKnob ("haf",    "FREQUENZ (KHZ)", true, "5|10|20");
    addKnob ("hatten", "ABSENKEN",       true);
    addKnob ("tube",   u8 ("R\xc3\x96HRE"), false);
    addKnob ("output", "AUSGANG",        false, u8 ("\xe2\x88\x92" "12|\xe2\x88\x92" "6|0|+6|+12"));
    attachSet (0);

    setupSwitch (onSwitch, "on", "AN", "AUS", lampRed);
    modeSwitch.setButtonText ("x");
    modeSwitch.getProperties().set ("onText", "M/S");
    modeSwitch.getProperties().set ("offText", "STEREO");
    modeSwitch.getProperties().set ("lamp", (int) lampAmber.getARGB());
    addAndMakeVisible (modeSwitch);
    // Modus ist ein Auswahl-Parameter (0/1) – als Schalter bedienen
    modeSwitch.onClick = [this]
    {
        if (auto* prm = proc.apvts.getParameter ("mode"))
            prm->setValueNotifyingHost (modeSwitch.getToggleState() ? 1.0f : 0.0f);
    };

    for (auto* b : { &midButton, &sideButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (1);
        addAndMakeVisible (*b);
    }
    midButton.setButtonText ("MITTE");
    sideButton.setButtonText ("SEITE");
    midButton.setToggleState (true, juce::dontSendNotification);
    midButton.onClick  = [this] { if (midButton.getToggleState())  { editSet = 0; attachSet (0); } };
    sideButton.onClick = [this] { if (sideButton.getToggleState()) { editSet = 1; attachSet (1); } };

    presetBox.getProperties().set ("plate", true);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    curve.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (curve);

    setSize (1100, 680);
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
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setMouseDragSensitivity (steps.isNotEmpty() && ! steps.contains ("SCHMAL") ? 140 : 240);
    if (steps.isNotEmpty())
        k->slider.getProperties().set ("stepLabels", steps);
    addAndMakeVisible (k->slider);
    if (! perSet)
        k->attachment = std::make_unique<SliderAttachment> (proc.apvts, baseID, k->slider);
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

void PultAudioProcessorEditor::setupSwitch (juce::ToggleButton& b, const juce::String& paramID,
                                           const juce::String& onText, const juce::String& offText, juce::Colour lamp)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lamp.getARGB());
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, b));
}

void PultAudioProcessorEditor::timerCallback()
{
    const bool ms = proc.isMidSide();
    modeSwitch.setToggleState (ms, juce::dontSendNotification);
    if (ms != shownMs)
    {
        shownMs = ms;
        midButton.setVisible (ms); sideButton.setVisible (ms);
        if (! ms && editSet != 0) { editSet = 0; midButton.setToggleState (true, juce::dontSendNotification); attachSet (0); }
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
void PultAudioProcessorEditor::renderTextures()
{
    tolexImage = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), false);
    {
        juce::Image::BitmapData data (tolexImage, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (7);
        for (int y = 0; y < tolexImage.getHeight(); ++y)
            for (int x = 0; x < tolexImage.getWidth(); ++x)
            {
                const int weave = ((x + y) % 4 == 0 || (x - y + 4096) % 4 == 0) ? -5 : 0;
                const int n = rnd.nextInt (13) - 6 + weave;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, 34 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 28 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 24 + n)));
            }
    }
    if (faceplate.isEmpty())
        return;
    faceplateImage = juce::Image (juce::Image::ARGB, faceplate.getWidth(), faceplate.getHeight(), true);
    juce::Graphics g (faceplateImage);
    const auto r = faceplateImage.getBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (cream.brighter (0.06f), 0, 0, creamDark, 0, r.getHeight(), false));
    g.fillRoundedRectangle (r, 6.0f);
    juce::Random rnd (3);
    for (int y = 0; y < faceplate.getHeight(); ++y)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xff6b5534)).withAlpha (0.015f + rnd.nextFloat() * 0.035f));
        g.drawHorizontalLine (y, 0.0f, r.getWidth());
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                             juce::Colour (0x30694a1c), 0, 0, true));
    g.fillRoundedRectangle (r, 6.0f);
}

void PultAudioProcessorEditor::drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                                            juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawFittedText (text, r.translated (0.0f, 1.0f).toNearestInt(), j, 2, 1.0f);
    g.setColour (ink);
    g.drawFittedText (text, r.toNearestInt(), j, 2, 1.0f);
}

void PultAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (tolexImage, 0, 0);

    const auto scriptF = lnf.scriptFont (64.0f);
    const auto logoArea = juce::Rectangle<float> (40.0f, 8.0f, 400.0f, 84.0f);
    g.setFont (scriptF);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("Klangbild", logoArea.translated (2.0f, 3.0f), juce::Justification::centredLeft);
    g.setColour (logo);
    g.drawText ("Klangbild", logoArea, juce::Justification::centredLeft);
    const float sw = (float) juce::GlyphArrangement::getStringWidthInt (scriptF, "Klangbild");
    g.setColour (gold);
    g.setFont (lnf.labelFont (26.0f, 0.35f));
    g.drawText ("PULT", juce::Rectangle<float> (logoArea.getX() + sw + 14.0f, logoArea.getY() + 30.0f, 140.0f, 40.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.55f));
    g.setFont (lnf.labelFont (12.0f, 0.3f));
    g.drawText (u8 ("PASSIVER R\xc3\x96HREN-EQ \xc2\xb7 STEREO \xc2\xb7 MITTE/SEITE"), juce::Rectangle<float> (44.0f, 90.0f, 460.0f, 16.0f), juce::Justification::centredLeft);

    const auto pa = presetArea.toFloat();
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText ("PROGRAMM", pa.withY (pa.getY() - 15.0f).withHeight (13.0f), juce::Justification::centredLeft);

    // Rahmen um das Messprotokoll
    const auto cb = curveBezel.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (cb.translated (2.0f, 3.0f), 10.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.25f), 0, cb.getY(), gold.darker (0.6f), 0, cb.getBottom(), false));
    g.fillRoundedRectangle (cb, 10.0f);

    // Frontplatte
    const auto fp = faceplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (fp.expanded (4.0f).translated (0, 2.0f), 9.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.3f), 0, fp.getY(), gold.darker (0.6f), 0, fp.getBottom(), false));
    g.fillRoundedRectangle (fp.expanded (3.0f), 8.0f);
    g.drawImageAt (faceplateImage, faceplate.getX(), faceplate.getY());
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (fp, 6.0f, 1.0f);
    for (auto [x, y, a] : { std::tuple { fp.getX() + 12.0f, fp.getY() + 12.0f, 0.4f }, std::tuple { fp.getRight() - 12.0f, fp.getY() + 12.0f, 1.3f },
                            std::tuple { fp.getX() + 12.0f, fp.getBottom() - 12.0f, 2.2f }, std::tuple { fp.getRight() - 12.0f, fp.getBottom() - 12.0f, 0.9f } })
        VintageLookAndFeel::drawScrew (g, { x, y }, 5.0f, a);

    // Gruppen mit Überschrift und feinem Rahmen
    const juce::String setName = shownMs ? (editSet == 0 ? u8 (" \xc2\xb7 MITTE") : u8 (" \xc2\xb7 SEITE")) : juce::String();
    const std::pair<juce::Rectangle<int>, juce::String> groups[] = {
        { groupLow,   "TIEFEN" + setName },
        { groupHigh,  u8 ("H\xc3\x96HEN ANHEBEN") + setName },
        { groupAtten, u8 ("H\xc3\x96HEN ABSENKEN") + setName },
        { groupOut,   "AUSGANG" },
    };
    for (auto& [r, title] : groups)
    {
        const auto rf = r.toFloat();
        g.setColour (ink.withAlpha (0.25f));
        g.drawRoundedRectangle (rf, 6.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawRoundedRectangle (rf.translated (0.0f, 1.0f), 6.0f, 1.0f);
        const auto tr = juce::Rectangle<float> (rf.getCentreX() - 120.0f, rf.getY() - 9.0f, 240.0f, 18.0f);
        const float tw = (float) juce::GlyphArrangement::getStringWidthInt (lnf.labelFont (13.0f, 0.22f), title) + 24.0f;
        g.setColour (cream);
        g.fillRect (tr.withSizeKeepingCentre (tw, 14.0f));
        drawEngraved (g, title, tr, lnf.labelFont (13.0f, 0.22f), juce::Justification::centred);
    }

    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        drawEngraved (g, k->name, { sb.getX() - 20.0f, sb.getBottom() - 2.0f, sb.getWidth() + 40.0f, 18.0f },
                      lnf.labelFont (13.0f, 0.14f), juce::Justification::centred);
    }

    const juce::String names[] = { "EQ", "MODUS", shownMs ? u8 ("BEARBEITEN") : juce::String() };
    for (size_t i = 0; i < 3; ++i)
        drawEngraved (g, names[i], switchLabels[i].toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centred);

    const auto tag = tagArea.toFloat();
    g.setColour (ink.withAlpha (0.85f));
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (gold);
    g.setFont (lnf.labelFont (11.0f, 0.2f));
    g.drawText ("MODELL KB-1A", tag, juce::Justification::centred);
}

void PultAudioProcessorEditor::resized()
{
    const int W = getWidth();
    presetArea = { 44, 128, 330, 32 };
    presetBox.setBounds (presetArea);
    curveBezel = { W - 560, 14, 530, 166 };
    curve.setBounds (curveBezel.reduced (7));

    faceplate = { 24, 196, W - 48, getHeight() - 196 - 20 };
    const auto in = faceplate.reduced (26, 26);

    // obere Reihe: Tiefen (3) | Höhen anheben (3)
    const int kw = 150, kh = 150;
    const int halfW = in.getWidth() / 2;
    groupLow  = { in.getX(), in.getY(), halfW - 10, kh + 40 };
    groupHigh = { in.getX() + halfW + 10, in.getY(), halfW - 10, kh + 40 };
    auto place3 = [&] (juce::Rectangle<int> grp, int first)
    {
        const int gap = (grp.getWidth() - 3 * kw) / 4;
        for (int i = 0; i < 3; ++i)
            knobs[(size_t) (first + i)]->slider.setBounds (grp.getX() + gap + i * (kw + gap), grp.getY() + 14, kw, kh);
    };
    place3 (groupLow, 0);
    place3 (groupHigh, 3);

    // untere Reihe: Höhen absenken (2) | Ausgang: Röhre, Pegel (2) | Schalter
    const int rowY = in.getY() + kh + 66;
    const int rowH = in.getBottom() - rowY;
    groupAtten = { in.getX(), rowY, 2 * kw + 60, rowH };
    groupOut   = { groupAtten.getRight() + 20, rowY, 2 * kw + 60, rowH };
    for (int i = 0; i < 2; ++i)
    {
        knobs[(size_t) (6 + i)]->slider.setBounds (groupAtten.getX() + 20 + i * (kw + 20), rowY + 14, kw, kh - 10);
        knobs[(size_t) (8 + i)]->slider.setBounds (groupOut.getX() + 20 + i * (kw + 20), rowY + 14, kw, kh - 10);
    }

    const int ctrlX = groupOut.getRight() + 20, ctrlW = in.getRight() - ctrlX;
    const int colW = ctrlW / 2;
    switchLabels[0] = { ctrlX, rowY + 6, colW, 16 };
    onSwitch.setBounds (ctrlX + (colW - 120) / 2, rowY + 26, 120, 40);
    switchLabels[1] = { ctrlX, rowY + 84, colW, 16 };
    modeSwitch.setBounds (ctrlX + (colW - 120) / 2, rowY + 104, 120, 40);
    switchLabels[2] = { ctrlX + colW, rowY + 6, colW, 16 };
    midButton.setBounds (ctrlX + colW + (colW - 100) / 2, rowY + 28, 100, 32);
    sideButton.setBounds (ctrlX + colW + (colW - 100) / 2, rowY + 68, 100, 32);
    tagArea = { ctrlX + colW + (colW - 130) / 2, rowY + rowH - 36, 130, 24 };
    dividerArea = {};

    midButton.setVisible (shownMs); sideButton.setVisible (shownMs);
    renderTextures();
}
