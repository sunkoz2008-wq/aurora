#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class AuroraAudioProcessor final : public juce::AudioProcessor
{
public:
    AuroraAudioProcessor();
    ~AuroraAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Aurora"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateEq();

    using Filt = juce::dsp::IIR::Filter<float>;
    using Coef = juce::dsp::IIR::Coefficients<float>;

    // Chaine : In -> EQ -> Saturation (2x OS) -> Chorus -> Compresseur -> Delay -> Reverb -> Auto-Pan/Largeur -> Out -> Limiteur
    juce::dsp::Gain<float> inGain, makeupGain, outGain;
    juce::dsp::ProcessorDuplicator<Filt, Coef> eqLow, eqMid, eqHigh;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 4 };
    juce::dsp::Reverb reverb;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::DryWetMixer<float> mixer;

    juce::SmoothedValue<float> delaySmooth;
    float lpL = 0.f, lpR = 0.f, panPhase = 0.f;
    float maxDelaySamples = 1.f;
    std::array<float, 7> eqCache {};

    std::atomic<float> *pIn, *pOut, *pMix,
        *pLowG, *pLowF, *pMidG, *pMidF, *pMidQ, *pHighG, *pHighF,
        *pSatType, *pDrive, *pSatMix,
        *pChoRate, *pChoDepth, *pChoMix,
        *pDlyTime, *pDlyFb, *pDlyTone, *pDlyPP, *pDlyMix,
        *pRevSize, *pRevDamp, *pRevWidth, *pRevMix,
        *pCmpThresh, *pCmpRatio, *pCmpAtk, *pCmpRel, *pCmpMakeup,
        *pPanRate, *pPanDepth, *pPanWidth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraAudioProcessor)
};
