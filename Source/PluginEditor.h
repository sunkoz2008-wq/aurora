#pragma once

#include "PluginProcessor.h"

class AuroraAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AuroraAudioProcessorEditor (AuroraAudioProcessor&);
    ~AuroraAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Cell
    {
        juce::Label label;
        // controles d'abord, attachements ensuite (detruits dans l'ordre inverse)
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::SliderParameterAttachment> sliderAtt;
        std::unique_ptr<juce::ComboBoxParameterAttachment> comboAtt;
        std::unique_ptr<juce::ButtonParameterAttachment> toggleAtt;
    };

    struct Section
    {
        juce::String prefix, title;
        std::vector<Cell*> cells;
        juce::Rectangle<int> area;
    };

    AuroraAudioProcessor& proc;
    std::vector<std::unique_ptr<Cell>> cells;
    std::vector<std::unique_ptr<Section>> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraAudioProcessorEditor)
};
