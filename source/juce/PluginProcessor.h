/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/
#pragma once

#ifndef ConvMaxSize
	#define ConvMaxSize 16384
#endif

#include <mutex>

#include "../JuceLibraryCode/JuceHeader.h"
#include "IRLoader.h"
#include "../hpeq/TimeDomainConvolution.h"

//==============================================================================
/**
*/
class HpeqAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    HpeqAudioProcessor();
    ~HpeqAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	void setIRFile(juce::File file);
	juce::File getIRFile() const;
	
private:

	void updateLiveIR();

private:

	juce::File irFile;

	IRLoader irLoader;

	TimeDomainConvolution<ConvMaxSize> tdConvolution;

	AConvolutionEngine * liveEngine{ &tdConvolution };

	std::mutex irSwapMutex;

	bool irNeedsSwap{ false }; // if true, we need to swap the live IR with a newly loaded IR

	std::unique_ptr<ImpulseResponse> cachedLiveIR{ nullptr };	// the ir
	std::unique_ptr<ImpulseResponse> cachedLoadedIR{ nullptr };


private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HpeqAudioProcessor)
};
