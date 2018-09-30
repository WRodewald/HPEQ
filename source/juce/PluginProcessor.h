/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/
#pragma once

#ifndef ConvMaxSize
	#define ConvMaxSize 131072
#endif


// use this to enable the work in progress parfilt implementation
//#define ENABLE_PARFILT_WIP


#include <mutex>
#include "../JuceLibraryCode/JuceHeader.h"
#include "IRLoader.h"
#include "../hpeq/TimeDomainConvolution.h"
#include "../hpeq/FFTConvolution.h"
#include "../hpeq/FFTPartConvolution.h"
#include "../hpeq/AFourierTransformFactory.h"
#include "JuceFourierTransform.h"

#ifdef ENABLE_PARFILT_WIP
#include "../hpeq/ParFiltConvolution.h"
#endif



/**
	Implements #AFourierTransformFactory FFT engine factory. It uses the FFT engine provided by JUCE for the implementation.
*/
class JuceFourierTransformFactory : public AFourierTransformFactory
{
protected:
	virtual AFourierTransform * createFourierTransform(unsigned int order) const override;
};

/**
	A listener interface for classes that want to be notified when a impulse response was changed.
*/
class ImpulseResponseUpdateListener
{
public:
	/**
		Is called when a impulse response was changed
		@param ir the new impulse response
	*/
	virtual void setUpdateIR(const ImpulseResponse & ir) = 0;
};

//==============================================================================
/**
*/
class HpeqAudioProcessor  : public AudioProcessor, public AudioProcessorParameter::Listener, public AsyncUpdater
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


	void setIRUpdateListener(ImpulseResponseUpdateListener * listener);
	
protected:
	// Inherited via Listener
	virtual void parameterValueChanged(int parameterIndex, float newValue) override;
	virtual void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

	// Inherited via AsyncUpdater
	virtual void handleAsyncUpdate() override;


private:


	/**
		runs impulse response pre processing
	*/
	void updateAndPreProcessIR();

	/**
		Checks if the main thread knows about a new impulse response. If so, 
		swaps the IR used in the audio thread with the one keept in the main thread
	*/
	void updateAudioThreadIR();

	/**
		Shedules a thread safe update of the impulse response used in the audio thread
	*/
	void sheduleUpdateForAudioThreadIR();

private:


	AFourierTransformFactory * engine { AFourierTransformFactory::installStaticFactory(new JuceFourierTransformFactory()) };

	juce::File irFile;

	IRLoader irLoader;

	TimeDomainConvolution<ConvMaxSize>  tdConvolution;
	FFTConvolution<ConvMaxSize>			fftConvolution;
	FFTPartConvolution<ConvMaxSize>		fftPartConvolution;
#ifdef ENABLE_PARFILT_WIP
	ParFiltConvolution					parFiltConvolution;
#endif

	/*
		the offline IR is the impulse response currently loaded but fe
	*/

	bool irNeedsSwap{ false }; 
	std::mutex irSwapMutex;

	ImpulseResponse offlineIR;

	std::unique_ptr<ImpulseResponse> cachedLiveIR{ nullptr };	// impulse response used in audio thread
	std::unique_ptr<ImpulseResponse> cachedSwapIR{ nullptr };	// impulse response used for sawpping the live one

	
	
	struct 
	{
		juce::AudioParameterBool	*monoIR;
		juce::AudioParameterBool	*normalize;
		juce::AudioParameterChoice	*lowFade;
		juce::AudioParameterChoice	*highFade;
		juce::AudioParameterChoice	*smooth;
				
		juce::AudioParameterBool *invert;
		juce::AudioParameterBool *minPhase;

		juce::AudioParameterChoice *engine;

		juce::AudioParameterInt * partitions;
	} parameters;

	// normally would be a list and something more sophisticated but in this case, we only need that one notification
	ImpulseResponseUpdateListener * irListener{ nullptr };

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HpeqAudioProcessor)

	
};

