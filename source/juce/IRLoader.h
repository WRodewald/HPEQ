#pragma once

#include "JuceHeader.h"
#include "../hpeq/ImpulseResponse.h"

/**
	Class handles audio I/O, and stores one impulse response as part of the plugin's current state. 
*/
class IRLoader
{
public:

	enum class ErrorCode
	{
		NoError = 0,
		ToLong = 1,
		Other  = 2
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