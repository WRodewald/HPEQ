/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
}

HpeqAudioProcessor::~HpeqAudioProcessor()
{
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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
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
	
	// thread savely update our cached impulse response
	updateLiveIR();

	// get pointers
	auto lRead  = buffer.getReadPointer(0);
	auto rRead  = buffer.getReadPointer(1);

	auto lWrite = buffer.getWritePointer(0);
	auto rWrite = buffer.getWritePointer(1);

	// process live IR
	liveEngine->process(lRead, rRead, lWrite, rWrite, buffer.getNumSamples());
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
	std::unique_ptr<XmlElement> xml = std::unique_ptr<XmlElement>(new XmlElement("IRFile"));
	xml->setAttribute("Path", irFile.getFullPathName());
	copyXmlToBinary(*xml, destData);

}

void HpeqAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{

	std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
	if (xmlState.get() != nullptr)
	{
		if (xmlState->hasTagName("IRFile"))
		{
			setIRFile(xmlState->getStringAttribute("Path"));
		}
	}
}

void HpeqAudioProcessor::setIRFile(juce::File file)
{

	this->irFile = file;
	irLoader.loadImpulseResponse(irFile);

	{
		std::lock_guard<std::mutex> lock(irSwapMutex);
	
		cachedLoadedIR = std::unique_ptr<ImpulseResponse>(new ImpulseResponse(irLoader.getImpulseResponse()));

		irNeedsSwap = true; 
	}
}

juce::File HpeqAudioProcessor::getIRFile() const
{
	return irFile;
}

void HpeqAudioProcessor::updateLiveIR()
{	
	std::lock_guard<std::mutex> lock(irSwapMutex);

	if (irNeedsSwap)
	{
		std::swap(cachedLiveIR, cachedLoadedIR);
		irNeedsSwap = false;

		tdConvolution.setImpulseResponse(cachedLiveIR.get());
	}
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HpeqAudioProcessor();
}
