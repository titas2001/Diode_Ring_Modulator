/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstdio>
#include <iostream>

//==============================================================================
DiodeRingModulatorAudioProcessor::DiodeRingModulatorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    // add control for the input sine amplitude
    audioTree(*this, nullptr, juce::Identifier("PARAMETERS"),
        { std::make_unique<juce::AudioParameterFloat>("controlM_ID","ControlM",juce::NormalisableRange<float>(24.0, 2000.0, 1.0),100.0),
        std::make_unique<juce::AudioParameterFloat>("controlC_ID","ControlC",juce::NormalisableRange<float>(0.01, 10.0, 0.01),1.0),
        std::make_unique<juce::AudioParameterFloat>("controlGain_ID","ControlGain",juce::NormalisableRange<float>(0.01, 70.0, 0.01),10.0),
        std::make_unique<juce::AudioParameterFloat>("useInput_ID", "UseInput",juce::NormalisableRange<float>(0.1, 1.1, 1.0),1.1)
        }),
    lowPassFilter(juce::dsp::IIR::Coefficients< float >::makeLowPass((48000.0 * 16.0), 20000.0))
#endif
{
    oversampling.reset(new juce::dsp::Oversampling<float>(2, 4, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
    audioTree.addParameterListener("controlGain_ID", this);
    audioTree.addParameterListener("controlC_ID", this);
    audioTree.addParameterListener("controlM_ID", this);
    audioTree.addParameterListener("useInput_ID", this);



    controlledM = 100.0;
    controlledC = 1.0;
    controlledGain = 10.0;
    controlledInput = 1.1;

}

DiodeRingModulatorAudioProcessor::~DiodeRingModulatorAudioProcessor()
{
    oversampling.reset();
}

//==============================================================================
const juce::String DiodeRingModulatorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DiodeRingModulatorAudioProcessor::acceptsMidi() const
{
    return false;
}

bool DiodeRingModulatorAudioProcessor::producesMidi() const
{
    return false;
}

bool DiodeRingModulatorAudioProcessor::isMidiEffect() const
{
    return false;
}

double DiodeRingModulatorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DiodeRingModulatorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DiodeRingModulatorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DiodeRingModulatorAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String DiodeRingModulatorAudioProcessor::getProgramName(int index)
{
    return {};
}

void DiodeRingModulatorAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void DiodeRingModulatorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{

    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    //oversampling->reset();
    //oversampling->initProcessing(static_cast<size_t> (samplesPerBlock));

    //juce::dsp::ProcessSpec spec;
    //spec.sampleRate = sampleRate * 16;
    //spec.maximumBlockSize = samplesPerBlock * 15;
    //spec.numChannels = getTotalNumOutputChannels();

    //lowPassFilter.prepare(spec);
    //lowPassFilter.reset();
    // Set the constants
    Fs = sampleRate;
    T = 1 / Fs;
    R = 1e3;
    Vp = 0.17;
    Ve = 0.023;
    Id = 2.52e-9;
    err = 0.1e-3; // err for stopping iterations
    // set controlled values to starting values (redundant maybe delit later)
    controlledM = 100.0;
    controlledC = 1.0;
    controlledGain = 10.0;
    controlledInput = 1.1;
    voutOld = 0;
    fc = 100;
    C = 10e-9;
    C_p = 10e-9;
    L = 0.8e0;
    R_a = 600;
    R_i = 50;
    R_m = 80;

    rhoM = R_m / (1 + R_m * C * Fs);
    rhoA = R_a / (1 + R_a * C * Fs);
    rhoI = R_i / (1 + R_i * C * Fs);
    alpha = 1 / (L * Fs);

    UPastExp = Vector9d::Zero();
    UExp = Vector9d::Zero();
    UTempExp = Vector9d::Zero();
    W = Vector9d::Zero();
    NFExp = Vector9d::Zero();
    Identity = Matrix9d::Identity();
    JExp = Matrix9d::Zero();
    HExp = Matrix9d::Zero();
    argumentC = 0.0;
    argumentM = 0.0;
}

void DiodeRingModulatorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool DiodeRingModulatorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void DiodeRingModulatorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto mainInputOutput = getBusBuffer(buffer, true, 0);

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    /***************************************************************************************/
    // 1. Fill a new array here with upsampled input
    //      a. zeros array of size buffer*N
    //      b. assign input value to every N'th sample in the zeros array
    // 2. Apply low pass
    // 3. Run the VCF
    // 4. Apply low pass again 
    // 5. For loop to downsample

    //R = controlledR;
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());


    //juce::dsp::AudioBlock<float> blockInput(buffer);
    //juce::dsp::AudioBlock<float> blockOutput = oversampling->processSamplesUp(blockInput);
    if (argumentC > 1.0)
        argumentC = argumentC - std::floor(argumentC);

    if (argumentM > 1.0)
        argumentM = argumentM - std::floor(argumentM);

    //updateFilter();
    //lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            if (controlledInput > 1.0) vin = controlledGain * std::sin(3.14159265359 * 2.0 * argumentM);
            else vin = controlledGain * buffer.getSample(channel, sample);
            
            c = 5.0* std::sin(3.14159265359 * 2.0 * argumentC);
            argumentC = argumentC + controlledC * T;
            argumentM = argumentM + controlledM * T;

            k = 0;
            

            
            W << rhoM * vin / R_m, 0.0, 0.0, -c, -c, c, c, 0.0, 0.0;

            UPastExp[0] = rhoM * C * Fs * UExp[0]; UPastExp[1] = rhoA* C* Fs* UExp[1]; UPastExp[2] = rhoI* C_p* Fs* UExp[2];
            UPastExp[3] = UPastExp[4] = UPastExp[5] = UPastExp[6] = 0.0; UPastExp[7] = UExp[7]; UPastExp[8] = UExp[8];

            UTempExp[1] = UExp[1] + 1.0; // ensures that inner cycle starts
            
            while (std::abs(UExp[1] - UTempExp[1]) >= err && k < 1000) {
                
                for (int i = 0; i < 9; ++i) UTempExp[i] = UExp[i];
                UTempExp = UExp;


                JExp.row(0) << 0.0, 0.0, 0.0, -rhoM / 2.0 * gdExpDiff(UTempExp[3]), rhoM / 2.0 * gdExpDiff(UTempExp[4]), -rhoM / 2.0 * gdExpDiff(UTempExp[5]), rhoM / 2.0 * gdExpDiff(UTempExp[6]), rhoM, 0.0;
                JExp.row(1) << 0.0, 0.0, 0.0, rhoA / 2.0 * gdExpDiff(UTempExp[3]), -rhoA / 2.0 * gdExpDiff(UTempExp[4]), -rhoA / 2.0 * gdExpDiff(UTempExp[5]), rhoA / 2.0 * gdExpDiff(UTempExp[6]), 0.0, rhoA;
                JExp.row(2) << 0.0, 0.0, 0.0, rhoI * gdExpDiff(UTempExp[3]), rhoI * gdExpDiff(UTempExp[4]), -rhoI * gdExpDiff(UTempExp[5]), -rhoI * gdExpDiff(UTempExp[6]), 0.0, 0.0;
                JExp.row(3) << 1.0 / 2.0, -1.0 / 2.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
                JExp.row(4) << -1.0/2.0, 1.0/2.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;    
                JExp.row(5) << 1.0/2.0, 1.0/2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
                JExp.row(6) << (-1.0 / 2.0), (-1.0/2.0), 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
                JExp.row(7) << -alpha, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
                JExp.row(8) << 0.0, -alpha, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;

                

                NFExp[0] = rhoM * (-gdExp(UTempExp[3]) / 2.0 + gdExp(UTempExp[4]) / 2.0 - gdExp(UTempExp[5]) / 2.0 + gdExp(UTempExp[6]) / 2.0 + UTempExp[7]);
                NFExp[1] = rhoA * (gdExp(UTempExp[3]) / 2.0 - gdExp(UTempExp[4]) / 2.0 - gdExp(UTempExp[5]) / 2.0 + gdExp(UTempExp[6]) / 2.0 + UTempExp[8]);
                NFExp[2] = rhoI * (gdExp(UTempExp[3]) + gdExp(UTempExp[4]) - gdExp(UTempExp[5]) - gdExp(UTempExp[6]));
                NFExp[3] = UTempExp[0] / 2.0 - UTempExp[1] / 2.0 - UTempExp[2];
                NFExp[4] = -UTempExp[0] / 2.0 + UTempExp[1] / 2.0 - UTempExp[2];
                NFExp[5] = UTempExp[0] / 2.0 + UTempExp[1] / 2.0 + UTempExp[2];
                NFExp[6] = -UTempExp[0] / 2.0 - UTempExp[1] / 2.0 + UTempExp[2];
                NFExp[7] = -alpha * UTempExp[0];
                NFExp[8] = -alpha * UTempExp[1];


                
                Matrix4Inversion = (Identity - JExp);
                MultArray = UTempExp - NFExp - (W + UPastExp);
                MultProduct = Matrix4Inversion.inverse() * MultArray;
                UExp = UTempExp - MultProduct;

                ++k;
            }


            vout = limiter(UExp[1]*0.1);

            buffer.setSample(channel, sample, vout);
        }
    }
    //updateFilter();
    //lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));

    //oversampling->processSamplesDown(blockInput);

}
//==============================================================================
bool DiodeRingModulatorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}
void DiodeRingModulatorAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    //Parameters update  when sliders moved
    if (parameterID == "controlM_ID") {
        controlledM = newValue;
    }
    else if (parameterID == "controlC_ID") {
        controlledC = newValue;
    }
    else if (parameterID == "controlGain_ID") {
        controlledGain = newValue;
    }
    else if (parameterID == "useInput_ID") {
        controlledInput = newValue;
    }
}
void DiodeRingModulatorAudioProcessor::updateFilter()
{
    float frequency = 48e3 * 16;

    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(frequency, frequency / 16);
}
double DiodeRingModulatorAudioProcessor::gdExp(double vc)
{
    return Id * (std::exp(vc / (2 * Ve)) - 1);
}
double DiodeRingModulatorAudioProcessor::gdExpDiff(double vc)
{
    return (Id * std::exp(vc / (2 * Ve))) / (2 * Ve);
}
double DiodeRingModulatorAudioProcessor::limiter(double val)
{
    if (val < -1)
    {
        val = -1;
        return val;
    }
    else if (val > 1)
    {
        val = 1;
        return val;
    }
    return val;
}

juce::AudioProcessorEditor* DiodeRingModulatorAudioProcessor::createEditor()
{
    return new DiodeRingModulatorAudioProcessorEditor(*this, audioTree);
}

//==============================================================================
void DiodeRingModulatorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DiodeRingModulatorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DiodeRingModulatorAudioProcessor();
}
