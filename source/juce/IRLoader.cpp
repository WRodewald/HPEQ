#include "IRLoader.h"
#include "../hpeq/IRTools.h"

IRLoader::ErrorCode IRLoader::loadImpulseResponse(juce::File file, unsigned int maxSupportedLength)
{
	try
	{
		WavAudioFormat wavAudio;
		std::unique_ptr<AudioFormatReader> reader(wavAudio.createReaderFor(new FileInputStream(file), true));
		
		AudioSampleBuffer buffer(reader->numChannels, reader->lengthInSamples);
		reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);

		if (reader->lengthInSamples > maxSupportedLength) return ErrorCode::ToLong;

		this->loadedIR = ImpulseResponse(buffer.getReadPointer(0), buffer.getReadPointer(1), buffer.getNumSamples(), reader->sampleRate);
		IRTools::zeroPadToPow2(loadedIR);

		return ErrorCode::NoError;
	}
	catch (const std::exception & e)
	{}
	return ErrorCode::Other;
}

ImpulseResponse IRLoader::getImpulseResponse() const
{
	return loadedIR;
}
