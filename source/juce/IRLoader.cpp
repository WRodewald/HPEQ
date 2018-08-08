#include "IRLoader.h"

void IRLoader::loadImpulseResponse(juce::File file)
{
	
	WavAudioFormat wavAudio;
	std::unique_ptr<AudioFormatReader> reader(wavAudio.createReaderFor(new FileInputStream(file), true));

	AudioSampleBuffer buffer(reader->numChannels, reader->lengthInSamples);
	reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);
		
	this->loadedIR = ImpulseResponse(buffer.getReadPointer(0), buffer.getReadPointer(1), buffer.getNumSamples());
}

ImpulseResponse IRLoader::getImpulseResponse() const
{
	return loadedIR;
}
