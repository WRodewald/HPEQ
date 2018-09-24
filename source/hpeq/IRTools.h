#pragma once


#include "ImpulseResponse.h"

/**

	A collection of static functions for impulse response modifications
*/
class IRTools
{
public:

	/**
		Function removes stereo information from the impulse response
		@param ir the impulse response
	*/
	static void makeMono(ImpulseResponse & ir);

	/**
	Function smoothes the frequency response with a octave-width band
		@param ir		the impulse response
		@param width	kernel width in octaves
		@param fs		the sample rate
	*/
	static void octaveSmooth(ImpulseResponse & ir, float width, float fs);

	/**
		Function normalizes the frequency response overall amplitude
		Only works with impulse responses with size 2^n.
		@param ir the impulse response
		@param fs the sample rate
	*/
	static void normalize(ImpulseResponse & ir, float fs);

	/**
	Function fades out the frequency response at very low and high frequencies.
	Only works with impulse responses with size 2^n.
	@param ir the impulse response
	@param fs the sample rate
	*/
	static void fadeOut(ImpulseResponse & ir, float fs);


	/**
		Function converts the impulse respons to a minimum phase system. 
		Only works with impulse responses with size 2^n.
		@param ir the impulse response
	*/
	static void makeMinPhase(ImpulseResponse & ir);

	/**
		Function inverts the amplitude magnitude response. 
		Only works with impulse responses with size 2^n.
		@param ir the impulse response
	*/
	static void invertMagResponse(ImpulseResponse & ir);


	/**
		Function zero pads to get an size 2^n impulse response.
		@param ir the impulse response
	*/
	static void zeroPadToPow2(ImpulseResponse & ir);
	
	/**
		Function returns the smallest power of 2 value y = 2^n so that y >= x.
	*/
	static unsigned int nextPow2(unsigned int x);

	/**
		Function returns true if the size of the impulse response is a power of 2
	*/
	static bool isPow2(const ImpulseResponse & ir);

private:


	/**
		Function calculates a frequency wheight in terms of a Nth order 2nd-order band pass (1st order high pass / lowpass in sereis)
		@param f frequency to calculate the weight at
		@param fs the sample rate
		@param fLP lowpass frequency
		@param fHP highpass frequency
		@param order the order of the filters H = (HLP^order * HHP^order)
	*/
	static float frequencyWeight(float f, float fs, float fHP, float fLP, unsigned int order);
	
};