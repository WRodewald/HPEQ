/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/



#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../hpeq/IRTools.h"



//==============================================================================
HpeqAudioProcessor::HpeqAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	
	unsigned int pid = 0;
	addParameter(parameters.invert		= new AudioParameterBool("Invert",		"Invert IR", 0));
	addParameter(parameters.minPhase	= new AudioParameterBool("MinPhase",	"Min Phase", 0));
	addParameter(parameters.normalize	= new AudioParameterBool("Norm",		"Normalize", 0));
	addParameter(parameters.lowFade		= new AudioParameterChoice("LowFade",	"Low Fade",	 { "Off", "20Hz", "50Hz", "100Hz" },0));
	addParameter(parameters.highFade	= new AudioParameterChoice("HighFade",	"High Fade", { "Off", "5kHz", "10kHz", "20kHz" }, 0));
	addParameter(parameters.monoIR		= new AudioParameterBool("Mono",		"Mono IR", 0));
	addParameter(parameters.smooth		= new AudioParameterChoice("Smooth",	"Smooth", { "Off", "1/12 Octave", "1/5 Octave", "1/3 Octave", "1 Octave" }, 0));
	addParameter(parameters.engine		= new AudioParameterChoice("Engine", "Engine", { "Time Domain", "Brute FFT", "Part FFT", "ParFilt" }, 0));

	addParameter(parameters.partitions	= new AudioParameterInt("Partitions",	"Partitions",0,7,0));

	addParameter(parameters.parFiltWarp  = new AudioParameterFloat("ParFiltWarp", "ParFilt War", 0, 0.9, .50));
	addParameter(parameters.parFiltIIROrder = new AudioParameterChoice("ParFiltIIROrder", "ParFilt IIR Order", { "16","32", "64", "128" },1));
	addParameter(parameters.parFiltFIROrder = new AudioParameterChoice("ParFiltFIROrder", "ParFilt FIR Order", { "0","1", "2", "4", "8" }, 1));
	
	parameters.minPhase->addListener(this);
	parameters.monoIR->addListener(this);
	parameters.invert->addListener(this);
	parameters.normalize->addListener(this);
	parameters.lowFade->addListener(this);
	parameters.highFade->addListener(this);
	parameters.smooth->addListener(this);
	
	parameters.engine->addListener(this);

	parameters.partitions->addListener(this);

	parameters.parFiltWarp->addListener(this);
	parameters.parFiltIIROrder->addListener(this);
	parameters.parFiltFIROrder->addListener(this);

	

	startTimer(30);


	shedulePreProcessAndUpdateIR();
}

HpeqAudioProcessor::~HpeqAudioProcessor()
{
	joinPreProcessorThread();
}

//==============================================================================
const String HpeqAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HpeqAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HpeqAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HpeqAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HpeqAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HpeqAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int HpeqAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HpeqAudioProcessor::setCurrentProgram (int index)
{
}

const String HpeqAudioProcessor::getProgramName (int index)
{
    return {};
}

void HpeqAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void HpeqAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	auto errorCode = irLoader.updateSampleRate(sampleRate, ConvMaxSize);

	if (errorCode != IRLoader::ErrorCode::NoError)
	{
		if (auto editor = dynamic_cast<HpeqAudioProcessorEditor*>(getActiveEditor()))
		{
			auto messageCode = (errorCode == IRLoader::ErrorCode::ToLong)
				? HpeqAudioProcessorEditor::ErrorMessageType::IRToLong
				: HpeqAudioProcessorEditor::ErrorMessageType::IRCouldNotLoad;
			editor->displayErrorMessage(messageCode);
		}
	}
}

void HpeqAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HpeqAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void HpeqAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
	

	// get pointers
	auto lRead  = buffer.getReadPointer(0);
	auto rRead  = buffer.getReadPointer(1);

	auto lWrite = buffer.getWritePointer(0);
	auto rWrite = buffer.getWritePointer(1);


	auto currentEngine = parameters.engine->getCurrentChoiceName();

	if (currentEngine == "Time Domain")
	{
		this->tdConvolution.process(lRead, rRead, lWrite, rWrite, buffer.getNumSamples());
	}
	else if (currentEngine == "Brute FFT")
	{
		this->fftConvolution.process(lRead, rRead, lWrite, rWrite, buffer.getNumSamples());
	}
	else if (currentEngine == "Part FFT")
	{
		this->fftPartConvolution.process(lRead, rRead, lWrite, rWrite, buffer.getNumSamples());
	}
	else if (currentEngine == "ParFilt")
	{
		this->parFiltConvolution.process(lRead, rRead, lWrite, rWrite, buffer.getNumSamples());
	}

}

//==============================================================================
bool HpeqAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* HpeqAudioProcessor::createEditor()
{
    return new HpeqAudioProcessorEditor (*this);
}

//==============================================================================
void HpeqAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	std::unique_ptr<XmlElement> xml = std::unique_ptr<XmlElement>(new XmlElement("State"));
	xml->setAttribute("IRPath", irFile.getFullPathName());

	for (auto param : getParameters())
	{
		auto paramWID = dynamic_cast<AudioProcessorParameterWithID*>(param);
		if (paramWID) xml->setAttribute(paramWID->paramID, paramWID->getValue());
	}
	
	copyXmlToBinary(*xml, destData);

}

void HpeqAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{

	std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
	if (xmlState.get() != nullptr)
	{
		if (xmlState->hasTagName("State"))
		{
			if(xmlState->hasAttribute("IRPath") && (xmlState->getStringAttribute("IRPath") != ""))
				setIRFile(xmlState->getStringAttribute("IRPath"));

			for (int i = 0; i < xmlState->getNumAttributes(); i++)
			{
				for (auto param : getParameters())
				{
					auto paramWID = dynamic_cast<AudioProcessorParameterWithID*>(param);
					if (!paramWID) continue;
					
					if (paramWID->paramID == xmlState->getAttributeName(i))
					{						
						float val = std::atof(xmlState->getAttributeValue(i).toStdString().c_str());
						paramWID->setValueNotifyingHost(val);

					}
				}
			}
		}
	}

}

void HpeqAudioProcessor::setIRFile(juce::File file)
{

	this->irFile = file;
	auto errorCode = irLoader.loadImpulseResponse(irFile, ConvMaxSize);
	
	if (errorCode == IRLoader::ErrorCode::NoError)
	{
		shedulePreProcessAndUpdateIR();
	}
	else
	{
		if (auto editor = dynamic_cast<HpeqAudioProcessorEditor*>(getActiveEditor()))
		{
			auto messageCode = (errorCode == IRLoader::ErrorCode::ToLong) 
				? HpeqAudioProcessorEditor::ErrorMessageType::IRToLong 
				: HpeqAudioProcessorEditor::ErrorMessageType::IRCouldNotLoad;
			editor->displayErrorMessage(messageCode);
		}
	}
}

juce::File HpeqAudioProcessor::getIRFile() const
{
	return irFile;
}

HpeqAudioProcessor::BusyState HpeqAudioProcessor::getBusyState()
{
	return checkPreProcessorState();
}

void HpeqAudioProcessor::setIRUpdateListener(ImpulseResponseUpdateListener * listener)
{
	this->irListener = listener;
	if (irListener)
	{
		irListener->setUpdateIR(impulseResponse);
	}
}

void HpeqAudioProcessor::shedulePreProcessAndUpdateIR()
{
	std::map<juce::String, float> lowFadeFreqMap{ 
			{"Off",		  0},
			{"20Hz",	 20},
			{"50Hz",	 50},
			{"100Hz",	100} };
	
		std::map<juce::String, float> highFadeFreqMap{
			{ "Off",	    0 },
			{ "5kHz",	 5000 },
			{ "10kHz",	10000 },
			{ "20kHz",	20000} };

		std::map<juce::String, float> smoothValueMap{
			{ "Off",			0.f },
			{ "1/12 Octave",	1.f / 12.f },
			{ "1/5 Octave",		1.f / 5.f },
			{ "1/3 Octave",		1.f / 3.f },
			{ "1 Octave",		1.f } };

		
		std::map<juce::String, Engine> engineValueMap {
			{ "Time Domain",	Engine::TimeDomain},
			{ "Brute FFT",		Engine::FFTBrute},
			{ "Part FFT",		Engine::FFTPartitioned},
			{ "ParFilt",		Engine::ParFilt} };

	// get impulse response from file
	impulseResponse = irLoader.getImpulseResponse();

	// collect configuration
	PreProcessorConfig cfg;

	cfg.mono		= parameters.monoIR->get();
	cfg.normalize	= parameters.normalize->get();
	cfg.minPhase	= parameters.minPhase->get();
	cfg.invert		= parameters.invert->get();

	cfg.octaveSmoothWidth = smoothValueMap[parameters.smooth->getCurrentValueAsText()];
	cfg.lowFadeFreq		  = lowFadeFreqMap[parameters.lowFade->getCurrentValueAsText()];
	cfg.highFadeFreq	  = highFadeFreqMap[parameters.highFade->getCurrentValueAsText()];
	
	cfg.lowFade  = cfg.lowFadeFreq != 0;
	cfg.highFade = cfg.highFadeFreq	!= 0;

	cfg.engine = engineValueMap[parameters.engine->getCurrentValueAsText()];

	cfg.parFiltFIROrder = std::atoi(parameters.parFiltFIROrder->getCurrentValueAsText().toStdString().c_str());
	cfg.parFiltNumSOS = std::atoi(parameters.parFiltIIROrder->getCurrentValueAsText().toStdString().c_str());

	cfg.parFiltWarp  = parameters.parFiltWarp->get();

	cfg.fftPartitions = parameters.partitions->get();
	
	auto input = std::unique_ptr<PreProcessorInput>(new PreProcessorInput);
	input->cfg = cfg;
	input->ir = irLoader.getImpulseResponse();

	this->sheduledPreProcessorInput = std::move(input);

	// check right now if we can start the pre processor
	checkPreProcessorState();	
}

HpeqAudioProcessor::BusyState  HpeqAudioProcessor::checkPreProcessorState()
{
	using namespace std::chrono;
	
	BusyState busyState = BusyState::Idle;

	if (futurePreProcessorOutput != nullptr)
	{
		auto state = futurePreProcessorOutput->wait_for(1us);

		if (state == std::future_status::ready)
		{
			this->impulseResponse = futurePreProcessorOutput->get();
			futurePreProcessorOutput = nullptr;

			if (irListener) irListener->setUpdateIR(impulseResponse);
			
			busyState = BusyState::Idle;
		}
		else
		{
			busyState = BusyState::Busy;
		}
	}

	
	// we can start a new pre processing run?
	if ((futurePreProcessorOutput == nullptr) && (sheduledPreProcessorInput != nullptr))
	{
		// get config but reset the stored one
		auto input = *sheduledPreProcessorInput;
		sheduledPreProcessorInput = nullptr;

		// get ir
		auto ir = irLoader.getImpulseResponse();

		// future from a packaged_task
		std::packaged_task<ImpulseResponse()> task( [this, input]() { return preProcessAndUpdateIR(input.ir, input.cfg); });
		futurePreProcessorOutput = std::unique_ptr<std::future<ImpulseResponse>>(new std::future<ImpulseResponse>(task.get_future()));
		std::thread(std::move(task)).detach(); // launch on a thread

		busyState = BusyState::Busy;
	}
	return busyState;
}

void HpeqAudioProcessor::joinPreProcessorThread()
{
	if (futurePreProcessorOutput != nullptr)
	{
		futurePreProcessorOutput->wait();
		futurePreProcessorOutput = nullptr;
	}
}

ImpulseResponse HpeqAudioProcessor::preProcessAndUpdateIR(ImpulseResponse ir, PreProcessorConfig cfg)
{
	if (cfg.mono) IRTools::makeMono(ir);

	if (cfg.invert) IRTools::invertMagResponse(ir);

	if (cfg.lowFade ||cfg.highFade)
	{
		IRTools::fadeOut(ir, cfg.lowFadeFreq, cfg.highFadeFreq, cfg.lowFade ? 2 : 0, cfg.highFade ? 2 : 0);
	}

	if (cfg.octaveSmoothWidth != 0) IRTools::octaveSmooth(ir, cfg.octaveSmoothWidth);

	if (cfg.normalize) IRTools::normalize(ir);

	if (cfg.minPhase) IRTools::makeMinPhase(ir);

	
	
	parFiltConvolution.setFilterBankSize(cfg.parFiltNumSOS, cfg.parFiltFIROrder);
	parFiltConvolution.setWarpCoefficient(cfg.parFiltWarp);

	// partFilt actually does some heavy analysis. We only notify it about IR change when it's currently active
	if (cfg.engine == Engine::ParFilt)
	{
		parFiltConvolution.setImpulseResponse(ir);
	}
	
	tdConvolution.setImpulseResponse(ir);
	fftConvolution.setImpulseResponse(ir);

	fftPartConvolution.setPartitioningOrder(cfg.fftPartitions);
	fftPartConvolution.setImpulseResponse(ir);
	
	return ir;
}

void HpeqAudioProcessor::timerCallback()
{
	checkPreProcessorState();
}

void HpeqAudioProcessor::handleAsyncUpdate()
{
	shedulePreProcessAndUpdateIR();
}

void HpeqAudioProcessor::parameterValueChanged(int parameterIndex, float newValue)
{
	triggerAsyncUpdate();
}

void HpeqAudioProcessor::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
{
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HpeqAudioProcessor();
}

AFourierTransform * JuceFourierTransformFactory::createFourierTransform(unsigned int order) const
{
	return new JuceFourierTransform(order);
}
