#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;

Attr unit (const char* u, int decimals, bool sign = false)
{
    return Attr().withStringFromValueFunction ([=] (float v, int)
                 {
                     auto s = juce::String (std::abs (v) < 0.05f && decimals > 0 ? 0.0f : v, decimals).replaceCharacter ('.', ',');
                     if (sign && v > 0.049f) s = "+" + s;
                     return (s + " " + u).trim();
                 })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.replaceCharacter (',', '.').getFloatValue(); });
}
}

juce::AudioProcessorValueTreeState::ParameterLayout PultAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (int set = 0; set < 2; ++set)
    {
        const juce::String suffix = set == 0 ? "" : " (Seite)";
        auto knob = [&] (const char* id, const char* name)
        {
            l.add (std::make_unique<APF> (juce::ParameterID { pid (id, set), 1 }, juce::String (juce::CharPointer_UTF8 (name)) + suffix,
                                          juce::NormalisableRange<float> (0.0f, 10.0f, 0.05f), 0.0f, unit ("", 1)));
        };
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { pid ("lf", set), 1 }, "Tiefen-Frequenz" + suffix,
                                                             juce::StringArray { "20 Hz", "30 Hz", "60 Hz", "100 Hz" }, 2));
        knob ("lboost", "Tiefen anheben");
        knob ("latten", "Tiefen absenken");
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { pid ("hf", set), 1 }, juce::String (juce::CharPointer_UTF8 ("H\xc3\xb6hen-Frequenz")) + suffix,
                                                             juce::StringArray { "3 kHz", "4 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz", "16 kHz" }, 4));
        l.add (std::make_unique<APF> (juce::ParameterID { pid ("bw", set), 1 }, "Bandbreite" + suffix,
                                      juce::NormalisableRange<float> (0.0f, 10.0f, 0.05f), 5.0f, unit ("", 1)));
        knob ("hboost", "H\xc3\xb6hen anheben");
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { pid ("haf", set), 1 }, juce::String (juce::CharPointer_UTF8 ("Absenk-Frequenz H\xc3\xb6hen")) + suffix,
                                                             juce::StringArray { "5 kHz", "10 kHz", "20 kHz" }, 1));
        knob ("hatten", "H\xc3\xb6hen absenken");
    }
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Modus", juce::StringArray { "Stereo", "Mitte/Seite" }, 0));
    l.add (std::make_unique<APF> (juce::ParameterID { "tube", 1 }, juce::String (juce::CharPointer_UTF8 ("R\xc3\xb6hre")), juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, unit ("%", 0)));
    l.add (std::make_unique<APF> (juce::ParameterID { "output", 1 }, "Ausgang", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, unit ("dB", 1, true)));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "on", 1 }, "EQ an", true));
    return l;
}

const std::vector<PultAudioProcessor::Preset>& PultAudioProcessor::presets()
{
    // nicht genannte Parameter stehen auf ihrem Standardwert
    static const std::vector<Preset> p = {
        { "Neutral",                          {} },
        { "Bass-Trick 60 Hz",                 { { "lf", 2 }, { "lboost", 5 }, { "latten", 5 } } },
        { "Kick & Bass satt",                 { { "lf", 1 }, { "lboost", 6 }, { "latten", 3 }, { "hf", 2 }, { "bw", 4 }, { "hboost", 2 } } },
        { "Gesang mit Luft",                  { { "lf", 3 }, { "latten", 3 }, { "hf", 5 }, { "bw", 7 }, { "hboost", 4 }, { "haf", 2 }, { "hatten", 1 } } },
        { "Akustikgitarre",                   { { "lf", 3 }, { "lboost", 2 }, { "latten", 4 }, { "hf", 4 }, { "bw", 6 }, { "hboost", 3 } } },
        { "E-Gitarre: Glanz statt Schärfe",   { { "lf", 3 }, { "latten", 2 }, { "hf", 3 }, { "bw", 8 }, { "hboost", 2.5f }, { "haf", 1 }, { "hatten", 3 } } },
        { "Mastering sanft",                  { { "lf", 1 }, { "lboost", 3 }, { "latten", 2 }, { "hf", 5 }, { "bw", 8 }, { "hboost", 2.5f }, { "haf", 2 }, { "hatten", 1 }, { "tube", 15 } } },
        { "Mitte/Seite: breite Höhen",        { { "mode", 1 }, { "lf", 2 }, { "lboost", 3 }, { "latten", 2 }, { "hf_s", 5 }, { "bw_s", 8 }, { "hboost_s", 4 }, { "latten_s", 3 }, { "lf_s", 3 } } },
    };
    return p;
}

void PultAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (rp->paramID != "output" && rp->paramID != "on")
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

const juce::String PultAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

PultAudioProcessor::PultAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildPult", createLayout())
{
}

pult::Settings PultAudioProcessor::readSettings (int set) const
{
    auto v = [&] (const char* id) { return apvts.getRawParameterValue (pid (id, set))->load(); };
    pult::Settings s;
    s.lowFreq = (int) v ("lf");   s.lowBoost = v ("lboost");  s.lowAtten = v ("latten");
    s.highFreq = (int) v ("hf");  s.bandwidth = v ("bw");     s.highBoost = v ("hboost");
    s.attenFreq = (int) v ("haf"); s.highAtten = v ("hatten");
    return s;
}

bool PultAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out;
}

void PultAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (32, samplesPerBlock);
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> ((size_t) 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampler->initProcessing ((size_t) maxBlock);
    osLatency = (int) std::lround (oversampler->getLatencyInSamples());
    for (auto& d : dc) d.prepare (sr * 2.0);
    setLatencySamples (osLatency);
    work.setSize (2, maxBlock);
    direct.setSize (2, maxBlock);
    for (auto& d : osDelay) d.assign ((size_t) osLatency + 1, 0.0f);
    for (auto& d : dryDelay) d.assign ((size_t) osLatency + 1, 0.0f);
    osDelayPos = dryPos = 0;

    for (int s = 0; s < 2; ++s) { current[(size_t) s] = readSettings (s); section[(size_t) s] = pult::design (current[(size_t) s], sr); }
    for (auto& c : chan) c.reset();
    lastMs = isMidSide();

    smOut.reset (sr, 0.03);    smOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
    smOn.reset (sr, 0.02);     smOn.setCurrentAndTargetValue (apvts.getRawParameterValue ("on")->load() > 0.5f ? 1.0f : 0.0f);
    smTube.reset (sr, 0.05);   smTube.setCurrentAndTargetValue (apvts.getRawParameterValue ("tube")->load() * 0.01f);
    smUseOs.reset (sr, 0.02);  smUseOs.setCurrentAndTargetValue (smTube.getTargetValue() > 0.0f ? 1.0f : 0.0f);
    smMsSwap.reset (sr, 0.006); smMsSwap.setCurrentAndTargetValue (1.0f);
    vuCoef = (float) (1.0 - std::exp (-1.0 / (0.01 * sr)));
    vuState = 0;
}

void PultAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    const int nch = juce::jmin (2, buffer.getNumChannels());
    if (nch == 0 || oversampler == nullptr)
        return;

    for (int s = 0; s < 2; ++s)
    {
        const auto st = readSettings (s);
        if (! (st == current[(size_t) s])) { current[(size_t) s] = st; section[(size_t) s] = pult::design (st, sr); }
    }

    // Moduswechsel Stereo <-> Mitte/Seite: kurz ausblenden, im stillen Moment umschalten
    const bool wantMs = isMidSide() && nch == 2;
    if (wantMs != lastMs)
    {
        if (! smMsSwap.isSmoothing() && smMsSwap.getCurrentValue() <= 0.0f)
        {
            lastMs = wantMs;
            for (auto& c : chan) c.reset();
            smMsSwap.setTargetValue (1.0f);
        }
        else smMsSwap.setTargetValue (0.0f);
    }
    else smMsSwap.setTargetValue (1.0f);

    const float tubeTarget = apvts.getRawParameterValue ("tube")->load() * 0.01f;
    smTube.setTargetValue (tubeTarget);
    smUseOs.setTargetValue (tubeTarget > 0.0f ? 1.0f : 0.0f);
    smOut.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
    smOn.setTargetValue (apvts.getRawParameterValue ("on")->load() > 0.5f ? 1.0f : 0.0f);

    float* io[2] = { buffer.getWritePointer (0), nch > 1 ? buffer.getWritePointer (1) : nullptr };
    float pk = 0;
    const int olen = osLatency + 1;

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, total - start);
        float* w0 = work.getWritePointer (0); float* w1 = work.getWritePointer (1);

        // 1) passive Entzerrung (Stereo: L/R · M/S: Mitte und Seite getrennt)
        for (int i = 0; i < n; ++i)
        {
            const float l = io[0][start + i], r = io[1] != nullptr ? io[1][start + i] : l;
            if (lastMs)
            {
                const float m = 0.5f * (l + r), s = 0.5f * (l - r);
                const float mo = chan[0].process (section[0], m), so = chan[1].process (section[1], s);
                w0[i] = mo + so; w1[i] = mo - so;
            }
            else
            {
                w0[i] = chan[0].process (section[0], l);
                w1[i] = nch > 1 ? chan[1].process (section[0], r) : w0[i];
            }
        }

        // 2) Röhrenstufe, zweifach überabgetastet; ohne Röhre am Oversampling vorbei (nur gleich verzögert)
        float* d0 = direct.getWritePointer (0); float* d1 = direct.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            osDelay[0][(size_t) osDelayPos] = w0[i]; osDelay[1][(size_t) osDelayPos] = w1[i];
            const int rp = (osDelayPos + 1) % olen;
            d0[i] = osDelay[0][(size_t) rp]; d1[i] = osDelay[1][(size_t) rp];
            osDelayPos = rp;
        }
        juce::dsp::AudioBlock<float> block (work.getArrayOfWritePointers(), 2, (size_t) n);
        auto up = oversampler->processSamplesUp (block);
        const float t0 = smTube.getCurrentValue();
        smTube.skip (n);
        const float t1 = smTube.getCurrentValue();
        const size_t un = up.getNumSamples();
        for (size_t c = 0; c < 2; ++c)
        {
            float* d = up.getChannelPointer (c);
            for (size_t i = 0; i < un; ++i)
            {
                const float drive = t0 + (t1 - t0) * (float) i / (float) un;
                d[i] = dc[c].process (drive > 0.0f ? pult::tube (d[i], drive) : d[i]);
            }
        }
        oversampler->processSamplesDown (block);

        // 3) Ausgang, Bypass, Anzeige
        for (int i = 0; i < n; ++i)
        {
            const float u = smUseOs.getNextValue();
            const float a = d0[i] + (w0[i] - d0[i]) * u, b = d1[i] + (w1[i] - d1[i]) * u;
            const float go = smOut.getNextValue() * smMsSwap.getNextValue(), on = smOn.getNextValue();
            for (int c = 0; c < nch; ++c)
            {
                const float dryIn = io[c][start + i];
                dryDelay[(size_t) c][(size_t) dryPos] = dryIn;
                const float dry = dryDelay[(size_t) c][(size_t) ((dryPos + 1) % olen)];
                const float wet = (c == 0 ? a : b) * go;
                const float y = dry + (wet - dry) * on;
                io[c][start + i] = y;
                pk = juce::jmax (pk, std::abs (y));
            }
            dryPos = (dryPos + 1) % olen;
            const float av = nch > 1 ? 0.5f * (std::abs (io[0][start + i]) + std::abs (io[1][start + i])) : std::abs (io[0][start + i]);
            vuState += (av - vuState) * vuCoef;
        }
    }
    vuOut.store (vuState);
    peakOut.store (juce::jmax (pk, peakOut.load()));
}

void PultAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PultAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* PultAudioProcessor::createEditor() { return new kb::ScaledEditor (*this, new PultAudioProcessorEditor (*this)); }

#ifndef KB_EMBEDDED
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PultAudioProcessor(); }
#endif
