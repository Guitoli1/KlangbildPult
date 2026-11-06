#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PultDsp.h"

class PultAudioProcessor : public juce::AudioProcessor
{
public:
    PultAudioProcessor();
    ~PultAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameter-ID für Satz 0 (Stereo bzw. Mitte) oder 1 (Seite)
    static juce::String pid (const char* base, int set) { return set == 0 ? juce::String (base) : juce::String (base) + "_s"; }
    pult::Settings readSettings (int set) const;
    bool isMidSide() const { return apvts.getRawParameterValue ("mode")->load() > 0.5f; }
    double getSampleRateSafe() const { return sr; }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> vuOut { 0 }, peakOut { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<pult::Settings, 2> current;
    std::array<pult::Section, 2> section;
    std::array<pult::Channel, 2> chan;     // Stereo: L/R mit Satz 0 · M/S: M mit Satz 0, S mit Satz 1
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    std::array<pult::DcBlock, 2> dc;
    juce::AudioBuffer<float> work, direct;
    std::array<std::vector<float>, 2> osDelay; int osDelayPos = 0, osLatency = 0;
    std::array<std::vector<float>, 2> dryDelay; int dryPos = 0;
    int maxBlock = 512;
    bool lastMs = false;

    juce::SmoothedValue<float> smOut, smOn, smTube, smUseOs, smMsSwap;
    float vuState = 0, vuCoef = 0.001f;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PultAudioProcessor)
};
