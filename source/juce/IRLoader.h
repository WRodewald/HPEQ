#pragma once

#include "JuceHeader.h"
#include "../hpeq/ImpulseResponse.h"

/**
	Class handles reading from impulse response files and resamples and zero pads them to a size of N=2^n 
*/
class IRLoader
{
public:

	/**
		Error Codes that may be returned on loading or resampling
	*/
	enum class ErrorCode
	{
		NoError = 0, // no error
		ToLong = 1,	 // impulse response was too long during loading or resampling
		Other  = 2	 // other errors
	};


	/**
		Function loads an audio file.
		@param file The audio file.
		@param maxSupportedLength the maximum supported length of the impule response. If the length of the IR in file is longer, loadImpulseResponse will return false and not load the IR.
		@return The ErrorCode determining if the file could be loaded or why not.
	*/
	ErrorCode loadImpulseResponse(juce::File file, unsigned int maxSupportedLength);
	
	/**
		Function tries to resample the loaded impulse response. It may return an Error if the resulting impulse response would be to long.
		@param samplerate the new sample rate
		@param maxSupportedLength the maximum supported length of the impule response. If the length of the IR in file is longer, loadImpulseResponse will return false and not load the IR.
		@return The ErrorCode determining if the impulse response length is valid
	*/	
	ErrorCode updateSampleRate(float samplerate, unsigned int maxSupportedLength);




	/**
		Function returns the impulse response currently loaded.
	*/
	ImpulseResponse getImpulseResponse() const;


private:
	ImpulseResponse loadedIR;

	float samplerate{ 44100 };
};