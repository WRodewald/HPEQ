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

#include <mutex>
#include <future>

#include "../JuceLibraryCode/JuceHeader.h"

#include "IRLoader.h"

#include "../hpeq/TimeDomainConvolution.h"
#include "../hpeq/FFTConvolution.h"
#include "../hpeq/FFTPartConvolution.h"
#include "../hpeq/ParFiltConvolution.h"

#include "../hpeq/AFourierTransformFactory.h"
#include "JuceFourierTransform.h"

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
class HpeqAudioProcessor  : public AudioProcessor, public AudioProcessorParameter::Listener, public AsyncUpdater, public Timer
{
public:
	//==============================================================================

	enum class Engine
	{
		TimeDomain,
		FFTBrute,
		FFTPartitioned,
		ParFilt
	};

	enum class BusyState
	{
		Idle,
		Busy
	};


	// configuration for pre processing the IR
	struct PreProcessorConfig
	{
		bool mono;
		bool normalize;
		bool minPhase;
		bool invert;
		bool lowFade;
		bool highFade;

		float lowFadeFreq;
		float highFadeFreq;

		float octaveSmoothWidth;

		Engine engine;

		unsigned int fftPartitions;

		float	parFiltWarp;
		int		parFiltNumSOS;
		int		parFiltFIROrder;
		
	};

	struct PreProcessorInput
	{
		ImpulseResponse ir;
		PreProcessorConfig cfg;

	};

	
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

	BusyState getBusyState();

	void setIRUpdateListener(ImpulseResponseUpdateListener * listener);
	
protected:
	// Inherited via Listener
	virtual void parameterValueChanged(int parameterIndex, float newValue) override;
	virtual void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

	// Inherited via AsyncUpdater
	virtual void handleAsyncUpdate() override;

private:	
	/**
		Preprocesses the impulse response and notifies convolution engines
	*/
	ImpulseResponse preProcessAndUpdateIR(ImpulseResponse ir, PreProcessorConfig cfg);

	/**
		shedules a new pre processor run with the current parameters and loaded IR
	*/
	void shedulePreProcessAndUpdateIR();

	/**
		Two things: 
			- We check if the pre processor future object is ready to go, if so, we deploy the pre processed IR
			- We check if a new pre processor input was stored. If so, we start a new async thread that does the job
		@return returns 
	*/
	BusyState checkPreProcessorState();

	/**
		locks thread until preprocessor thread joined.	
	*/
	void	  joinPreProcessorThread();


private:
	
	AFourierTransformFactory * engine { AFourierTransformFactory::installStaticFactory(new JuceFourierTransformFactory()) };

	// convolution engines
	juce::File irFile;
	IRLoader irLoader;

	// convolution engines
	TimeDomainConvolution<ConvMaxSize>  tdConvolution;
	FFTConvolution<ConvMaxSize>			fftConvolution;
	FFTPartConvolution<ConvMaxSize>		fftPartConvolution;
	ParFiltConvolution					parFiltConvolution;

	// current impulse response
	ImpulseResponse impulseResponse;

	// we use this to store configurations for the pre processing until we can start a new thread.
	// A note: we could either just use a PreProcessorConfig together with a bool that tells us if the conf has changed 
	// or we could std::optional (C++17) that would do the same. Dynamic allocation is unnecessary
	std::unique_ptr<PreProcessorInput> sheduledPreProcessorInput;

	// will contain pre processed impulse response
	std::unique_ptr<std::future<ImpulseResponse>> futurePreProcessorOutput;
	

	struct 
	{
		juce::AudioParameterBool	*monoIR;
		juce::AudioParameterBool	*normalize;
		juce::AudioParameterChoice	*lowFade;
		juce::AudioParameterChoice	*highFade;
		juce::AudioParameterChoice	*smooth;
				
		juce::AudioParameterBool	*invert;
		juce::AudioParameterBool	*minPhase;

		juce::AudioParameterChoice *engine;

		juce::AudioParameterInt * partitions;

		juce::AudioParameterFloat  * parFiltWarp;
		juce::AudioParameterChoice  * parFiltIIROrder;
		juce::AudioParameterChoice  * parFiltFIROrder;

	} parameters;


	// normally would be a list and something more sophisticated but in this case, we only need that one notification
	ImpulseResponseUpdateListener * irListener{ nullptr };

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HpeqAudioProcessor)

	
	// Inherited via Timer
	virtual void timerCallback() override;

};

