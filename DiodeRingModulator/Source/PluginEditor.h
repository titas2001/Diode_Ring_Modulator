/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class DiodeRingModulatorAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Slider::Listener
{
public:
    DiodeRingModulatorAudioProcessorEditor(DiodeRingModulatorAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~DiodeRingModulatorAudioProcessorEditor() override;

    //==============================================================================
    void sliderValueChanged(juce::Slider* slider) override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DiodeRingModulatorAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& audioTree;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    double vout;
    juce::Slider controlGain, controlC, controlM, useInput;
    juce::Label labelGain, labelC, labelM, inputToggle, micInput, sineInput;

    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> useInputAttach;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachC;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachM;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiodeRingModulatorAudioProcessorEditor)
};
