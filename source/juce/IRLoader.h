#pragma once

#include "JuceHeader.h"
#include "../hpeq/ImpulseResponse.h"

/**
	Class handles audio I/O, and stores one impulse response as part of the plugin's current state. 
*/
class IRLoader
{
public:

	/**
		Function loads an audio file.
		\param file The audio file.
	*/
	void loadImpulseResponse(juce::File file);
		
	/**
		Function returns the mono impulse response currently stored.
	*/
	ImpulseResponse getImpulseResponse() const;


private:
	ImpulseResponse loadedIR;
};