#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

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
    void setupSwitch (juce::ToggleButton& b, const juce::String& paramID, const juce::String& onText, const juce::String& offText, juce::Colour lamp);
    void timerCallback() override;
    void renderTextures();
    void drawEngraved (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, juce::Justification);

    // Messprotokoll: Frequenzgang der Einstellungen
    struct Curve : public juce::Component
    {
        Curve (VintageLookAndFeel& l, PultAudioProcessor& p) : lnf (l), proc (p) {}
        VintageLookAndFeel& lnf; PultAudioProcessor& proc;
        std::array<pult::Settings, 2> shown; bool ms = false; int editSet = 0;
        void paint (juce::Graphics&) override;
    };

    PultAudioProcessor& proc;
    VintageLookAndFeel lnf;

    std::vector<std::unique_ptr<Knob>> knobs;
    juce::ToggleButton onSwitch, modeSwitch;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeAttachment;
    juce::TextButton midButton, sideButton;
    juce::ComboBox presetBox;
    Curve curve { lnf, proc };
    int editSet = 0;
    bool shownMs = false;

    juce::Rectangle<int> faceplate, presetArea, curveBezel, tagArea, dividerArea;
    juce::Rectangle<int> groupLow, groupHigh, groupAtten, groupOut;
    std::array<juce::Rectangle<int>, 3> switchLabels;
    juce::Image tolexImage, faceplateImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PultAudioProcessorEditor)
};
