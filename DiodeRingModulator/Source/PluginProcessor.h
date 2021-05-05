/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <Eigen/Dense>

//==============================================================================
/**
*/

class DiodeRingModulatorAudioProcessor : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    DiodeRingModulatorAudioProcessor();
    ~DiodeRingModulatorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    //==============================================================================
    //void setK(double val) { controlledK = val; };
    //void setVt(double val) { controlledR = val; };
    void parameterChanged(const juce::String& parameterID, float newValue);
    void updateFilter();
    double gdExp(double vc);
    double gdExpDiff(double vc);
    double limiter(double val);

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioProcessorValueTreeState audioTree;

private:
    double controlledGain, controlledC, controlledM, controlledInput;
    double Id, C, C_p, L, rhoM, rhoA, rhoI, R_m, R_a, R_i, alpha, Ve, Vp, R, err;
    double Fs, T;

    double fc, c;
    double vNom, vDenom;
    float vin;
    float vout, voutTemp, voutOld;
    int k;
    int upsamplingScale;
    double argumentC, argumentM;

    typedef Eigen::Matrix<double, 9, 9> Matrix9d;
    typedef Eigen::Matrix<double, 9, 1> Vector9d;

    Vector9d UExp, UPastExp, UTempExp, W, NFExp, MultArray, MultProduct;
    Matrix9d JExp, HExp, Identity, Matrix4Inversion;

    juce::dsp::ProcessorDuplicator< juce::dsp::IIR::Filter <float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiodeRingModulatorAudioProcessor)
};
