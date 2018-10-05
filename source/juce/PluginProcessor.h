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
#define ENABLE_PARFILT_WIP


#include <mutex>
#include <future>

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
class HpeqAudioProcessor  : public AudioProcessor, public AudioProcessorParameter::Listener, public AsyncUpdater, public Timer
{
public:
	//==============================================================================

	enum class Engine
	{
		TimeDomain,
		FFTBrute,
		FFTPartitioned,
#ifdef ENABLE_PARFILT_WIP
		ParFilt
#endif
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

#ifdef ENABLE_PARFILT_WIP
		float	parFiltWarp;
		int		parFiltOrder;
#endif
		bool operator== (const PreProcessorConfig &other)
		{
			// now this is going to be ugly.
			bool same = true;
			for (int i = 0; i < sizeof(PreProcessorConfig); i++)
			{
				same |= (*(((char*)this) + i)) == (*(((char*)&other) + i));
			}
			return same;
		}
	};

	struct PreProcessorInput
	{
		ImpulseResponse ir;
		PreProcessorConfig cfg;

	};

	// output from pre processor
	struct PreProcessorOutput
	{
		std::unique_ptr<ImpulseResponse> ir{ nullptr };
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
		Preprocesses the impulse response and parfilt analysis.
	*/
	PreProcessorOutput preProcess(ImpulseResponse ir, PreProcessorConfig cfg);

	/**
		Checks if the main thread knows about a new impulse response. If so, 
		swaps the IR used in the audio thread with the one keept in the main thread
	*/
	void updateAudioThreadIR();

	/**
		Shedules a thread safe update of the impulse response used in the audio thread
	*/
	void sheduleUpdateForAudioThreadIR(std::unique_ptr<ImpulseResponse> ir);

	/**
		shedules a new pre processor run with the current parameters and loaded IR
	*/
	void shedulePreProcessorRun();

	/**
		Two things: 
			- We check if the pre processor future object is ready to go, if so, we deploy the pre processed IR
			- We check if a new pre processor input was stored. If so, we start a new async thread that does the job
		@return returns 
	*/
	BusyState checkPreProcessorState();

private:
	
	AFourierTransformFactory * engine { AFourierTransformFactory::installStaticFactory(new JuceFourierTransformFactory()) };

	// convolution engines
	juce::File irFile;
	IRLoader irLoader;

	// convolution engines
	TimeDomainConvolution<ConvMaxSize>  tdConvolution;
	FFTConvolution<ConvMaxSize>			fftConvolution;
	FFTPartConvolution<ConvMaxSize>		fftPartConvolution;
#ifdef ENABLE_PARFILT_WIP
	ParFiltConvolution					parFiltConvolution;
#endif

	// main thread impulse response variable, we can use this without having to deal with threading
	ImpulseResponse offlineIR;

	// the followng two IRs are used to give a new IR to the audio thread by pointer swap
	// By keeping track of the swapped (old) one, we move the garbage collection to the main thread
	std::unique_ptr<ImpulseResponse> cachedLiveIR{ nullptr };	// impulse response used in audio thread
	std::unique_ptr<ImpulseResponse> cachedSwapIR{ nullptr };	// impulse response used for syncing with audio thread, th

	bool irNeedsSwap{ false }; // does main thread have a newer IR?
	std::mutex irSwapMutex;

	// we use this to store configurations for the pre processing until we can start a new thread.
	// A note: we could either just use a PreProcessorConfig together with a bool that tells us if the conf has changed 
	// or we could std::optional (C++17) that would do the same. Dynamic allocation is unnecessary
	std::unique_ptr<PreProcessorInput> futurePreProcessorInput;

	// we use this as an "anchorpoint" for pre processed IRs we want to give to the audio thread.
	// like with futureConfig, we ideally would used std::optional (if std::future does support this),
	// dynamic allocation is not necessary here
	std::unique_ptr<std::future<PreProcessorOutput>> futurePreProcessorOutput;

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

#ifdef ENABLE_PARFILT_WIP
		juce::AudioParameterFloat  * parFiltWarp;
		juce::AudioParameterInt  * parFiltOrder;
#endif

	} parameters;


	// normally would be a list and something more sophisticated but in this case, we only need that one notification
	ImpulseResponseUpdateListener * irListener{ nullptr };

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HpeqAudioProcessor)

	
	// Inherited via Timer
	virtual void timerCallback() override;

};

