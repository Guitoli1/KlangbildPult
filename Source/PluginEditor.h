#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

// Look im Stil eines 19-Zoll-Studiogeräts: blaugrauer Hammerschlaglack, weißer Siebdruck, schwarze Zeigerknöpfe
class RackLookAndFeel : public VintageLookAndFeel
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
};

class PultAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PultAudioProcessorEditor (PultAudioProcessor&);
    ~PultAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob { juce::Slider slider; juce::String name, baseID; bool perSet = false; std::unique_ptr<SliderAttachment> attachment; };

    Knob& addKnob (const juce::String& baseID, const juce::String& name, bool perSet, const juce::String& steps = {});
    void attachSet (int set);
    void setupToggle (juce::ToggleButton& b, const juce::String& onText, const juce::String& offText, juce::Colour lamp);
    void setCurveVisible (bool);
    void timerCallback() override;
    void renderPanel();

    // Messprotokoll: Frequenzgang der Einstellungen (ausklappbar)
    struct Curve : public juce::Component
    {
        Curve (VintageLookAndFeel& l, PultAudioProcessor& p) : lnf (l), proc (p) {}
        VintageLookAndFeel& lnf; PultAudioProcessor& proc;
        std::array<pult::Settings, 2> shown; bool ms = false; int editSet = 0;
        void paint (juce::Graphics&) override;
    };

    PultAudioProcessor& proc;
    RackLookAndFeel lnf;

    std::vector<std::unique_ptr<Knob>> knobs;
    juce::ToggleButton onSwitch, modeSwitch, editSwitch, curveSwitch;
    std::unique_ptr<ButtonAttachment> onAttachment;
    juce::ComboBox presetBox;
    Curve curve { lnf, proc };
    int editSet = 0;
    bool shownMs = false, curveVisible = false;

    static constexpr int rackW = 1280, rackH = 460, curveH = 210;
    juce::Rectangle<int> panel, nameplate, tagArea, curveBezel;
    std::array<juce::Rectangle<int>, 4> sections;
    std::array<juce::Rectangle<int>, 4> toggleLabels;
    juce::Image panelImage, tolexImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PultAudioProcessorEditor)
};
