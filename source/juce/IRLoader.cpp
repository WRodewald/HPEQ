#include "IRLoader.h"
#include "../hpeq/IRTools.h"

IRLoader::ErrorCode IRLoader::loadImpulseResponse(juce::File file, unsigned int maxSupportedLength)
{
	try
	{
		FileInputStream* stream = new FileInputStream(file);
		if (!stream->openedOk())
		{
			delete stream;
			return ErrorCode::Other;
		}

		WavAudioFormat wavAudio;
		std::unique_ptr<AudioFormatReader> reader(wavAudio.createReaderFor(stream, true));
		
		if (!reader) return ErrorCode::Other;
		AudioSampleBuffer buffer(reader->numChannels, reader->lengthInSamples);
		reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);
		
		auto rSource = (buffer.getNumChannels() > 1) ? 1 : 0;
		this->loadedIR = ImpulseResponse(buffer.getReadPointer(0), buffer.getReadPointer(rSource), buffer.getNumSamples(), reader->sampleRate);
		
		auto err = updateSampleRate(samplerate, maxSupportedLength);

		if (err != ErrorCode::NoError) return err;

		return ErrorCode::NoError;
	}
	catch (const std::exception & e)
	{}
	return ErrorCode::Other;
}

IRLoader::ErrorCode IRLoader::updateSampleRate(float samplerate, unsigned int maxSupportedLength)
{
	this->samplerate = samplerate;
	loadedIR = IRTools::resample(loadedIR, samplerate);

	if (loadedIR.getSize() > maxSupportedLength)
	{
		loadedIR = ImpulseResponse({ 1 }, { 1 }, samplerate);
		return ErrorCode::ToLong;
	}
	return ErrorCode::NoError;
}

ImpulseResponse IRLoader::getImpulseResponse() const
{
	auto irCopy = loadedIR;
	IRTools::zeroPadToPow2(irCopy);
	return irCopy;
}
