/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
DiodeRingModulatorAudioProcessorEditor::DiodeRingModulatorAudioProcessorEditor(DiodeRingModulatorAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), audioTree(vts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(450, 250);

    /*
 ColourIds{
  backgroundColourId = 0x1001200, thumbColourId = 0x1001300, trackColourId = 0x1001310, rotarySliderFillColourId = 0x1001311,
  rotarySliderOutlineColourId = 0x1001312, textBoxTextColourId = 0x1001400, textBoxBackgroundColourId = 0x1001500, textBoxHighlightColourId = 0x1001600,
  textBoxOutlineColourId = 0x1001700 }
*/

    controlGain.setColour(0x1001400, juce::Colour::fromRGBA(0x00, 0x40, 0x00, 0x80));
    controlGain.setColour(0x1001700, juce::Colour::fromRGBA(0x00, 0x00, 0x00, 0x00));
    controlM.setColour(0x1001400, juce::Colour::fromRGBA(0x00, 0x40, 0x00, 0x80));
    controlM.setColour(0x1001700, juce::Colour::fromRGBA(0x00, 0x00, 0x00, 0x00));
    controlC.setColour(0x1001400, juce::Colour::fromRGBA(0x00, 0x40, 0x00, 0x80));
    controlC.setColour(0x1001700, juce::Colour::fromRGBA(0x00, 0x00, 0x00, 0x00));

    // these define the parameters of our slider object

    controlGain.setSliderStyle(juce::Slider::LinearHorizontal);
    controlGain.addListener(this);
    controlGain.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    addAndMakeVisible(controlGain);
    sliderAttachGain.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioTree, "controlGain_ID", controlGain));
    //controlR.setPopupDisplayEnabled(true, false, this);
    labelGain.setText(("Input Gain"), juce::dontSendNotification);
    labelGain.setFont(juce::Font("Slope Opera", 16, 0));
    labelGain.setColour(juce::Label::textColourId, juce::Colour::fromRGBA(0x40, 0x40, 0x80, 0xff));
    addAndMakeVisible(labelGain);

    controlC.setSliderStyle(juce::Slider::LinearHorizontal);
    controlC.addListener(this);
    controlC.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    addAndMakeVisible(controlC);
    sliderAttachC.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioTree, "controlC_ID", controlC));
    //controlR.setPopupDisplayEnabled(true, false, this);
    labelC.setText(("Carier Frequency"), juce::dontSendNotification);
    labelC.setFont(juce::Font("Slope Opera", 16, 0));
    labelC.setColour(juce::Label::textColourId, juce::Colour::fromRGBA(0x40, 0x40, 0x80, 0xff));
    addAndMakeVisible(labelC);

    controlM.setSliderStyle(juce::Slider::LinearHorizontal);
    controlM.addListener(this);
    controlM.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    addAndMakeVisible(controlM);
    sliderAttachM.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioTree, "controlM_ID", controlM));
    //controlR.setPopupDisplayEnabled(true, false, this);
    labelM.setText(("Input Frequency"), juce::dontSendNotification);
    labelM.setFont(juce::Font("Slope Opera", 16, 0));
    labelM.setColour(juce::Label::textColourId, juce::Colour::fromRGBA(0x40, 0x40, 0x80, 0xff));
    addAndMakeVisible(labelM);


}

DiodeRingModulatorAudioProcessorEditor::~DiodeRingModulatorAudioProcessorEditor()
{
    sliderAttachC.reset();
    sliderAttachM.reset();
    sliderAttachGain.reset();
}

//==============================================================================
void DiodeRingModulatorAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::white);

    // set the current drawing colour to black
    g.setColour(juce::Colours::black);

    // set the font size and draw text to the screen
    g.setFont(15.0f);
    g.setFont(juce::Font("Slope Opera", 35.0f, 1));
    g.drawFittedText("NR Diode Ring Modulator", getLocalBounds(), juce::Justification::centred, 1);
}

void DiodeRingModulatorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    controlGain.setBounds(140, getHeight() - 60, getWidth() - 140, 20);
    labelGain.setBounds(0, getHeight() - 60, 140, 20);
    controlC.setBounds(140, getHeight() - 40, getWidth() - 140, 20);
    labelC.setBounds(0, getHeight() - 40, 140, 20);
    controlM.setBounds(140, getHeight() - 20, getWidth() - 140, 20);
    labelM.setBounds(0, getHeight() - 20, 140, 20);


}
void DiodeRingModulatorAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
}