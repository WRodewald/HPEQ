#include "IRLoader.h"
#include "../hpeq/IRTools.h"

void IRLoader::loadImpulseResponse(juce::File file)
{
	try
	{
		WavAudioFormat wavAudio;
		std::unique_ptr<AudioFormatReader> reader(wavAudio.createReaderFor(new FileInputStream(file), true));
		
		AudioSampleBuffer buffer(reader->numChannels, reader->lengthInSamples);
		reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);

		this->loadedIR = ImpulseResponse(buffer.getReadPointer(0), buffer.getReadPointer(1), buffer.getNumSamples(), reader->sampleRate);
		IRTools::zeroPadToPow2(loadedIR);
	}
	catch (const std::exception & e)
	{
		this->loadedIR = ImpulseResponse();
	}
}

ImpulseResponse IRLoader::getImpulseResponse() const
{
	return loadedIR;
}
