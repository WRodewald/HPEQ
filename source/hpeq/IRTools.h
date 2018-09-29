#pragma once


#include "ImpulseResponse.h"




/**

	A collection of static functions for impulse response modifications
*/
namespace IRTools
{

	/**
		Function resamples the impulse response. The implementation uses a sinc convolution approach. The concolution is windowed using a hanning window
		if the impulse response is longer then windowWidth. 
		@param ir the impulse response
		@param targetSampleRate the targeted sample rate
		@param windowWidth returns the width of the hanning window. If the impulse response is shorter then the window width, no window will be used
		and the convolution will be calculated over all samples.
		@return the resampled impulse response
	*/
	ImpulseResponse resample(const ImpulseResponse & ir, float targetSampleRate, unsigned int windowWidth = 64);

	/**
		Function converts the impulse response to a monophonic IR using the average of both channels.
		@param ir the impulse response
	*/
	void makeMono(ImpulseResponse & ir);

	/**
		Function removes silent trails. 
		@param ir the input impulse response
		@param dbThreshold sets the threshold in dB bellow which the tail will be truncated. The threshold is relative to the overall imulse response energy.
		@param keepPow2 if true, the impulse response will be zerro padded to the next power of 2 length
		@return the truncated impulse response
	*/
	ImpulseResponse truncate(const ImpulseResponse & ir, float dbThreshold, bool keepPow2);


	/**
	Function smoothes the frequency response with a octave-width band
		@param ir		the impulse response
		@param width	kernel width in octaves
		@param fs		the sample rate
	*/
	void octaveSmooth(ImpulseResponse & ir, float width, float fs);

	/**
		Function normalizes the frequency responses average magnitude response using a auditory weighting function similar to C-weighting.
		Only works with impulse responses with size 2^n.
		@param ir the impulse response
	*/
	void normalize(ImpulseResponse & ir);

	/**
	Function fades out the frequency response at very low and high frequencies towards the average magnitude response. The phase information is kept.
	@param ir the impulse response, has to be of length N = 2^n
	@param fHP the highpass frequency (lower fade out frequency)
	@param fLP the lowpass frequency (higher fade out frequency)
	@param hpfOrder the highpass order
	@param lpfOrder the lowpass order
	*/
	void fadeOut(ImpulseResponse & ir, float fHP, float fLP, unsigned int hpfOrder, unsigned int lpfOrder);


	/**
		Function converts the impulse respons to a minimum phase system. The implementation is exact but clips magnitude responses lower then -120 dB to prevent numerical issues or unstable systems.
		Only works with impulse responses with size 2^n.
		@param ir the impulse response, has to be of size N = 2^n
	*/
	void makeMinPhase(ImpulseResponse & ir);

	/**
		Function creates a warped version of the ImpulseResponse ir with warping coeff ir and length len
		@param ir the original impulse response
		@param lambda allpass / warping coefficient lambda
		@param len length of the output impulse response
		@return the warped impulse response
	*/
	ImpulseResponse warp(const ImpulseResponse & ir, float lambda, unsigned int len);


	/**
		Function inverts the amplitude magnitude response. 
		Only works with impulse responses with size 2^n.
		@param ir the impulse response, must be of size N=2^n
	*/
	void invertMagResponse(ImpulseResponse & ir);


	/**
		Function zero pads to get an size 2^n impulse response.
		@param ir the impulse response
	*/
	void zeroPadToPow2(ImpulseResponse & ir);
	
	/**
		Function returns the smallest power of 2 value y = 2^n so that y >= x.
	*/
	unsigned int nextPow2(unsigned int x);

	/**
		Function returns true if x is a power of 2
	*/
	bool isPow2(unsigned int x);

	/**
		Function returns the integer log2 for x = 2^n.
	*/
	constexpr unsigned int staticLog2(unsigned int x)
	{
		return (x <= 1) ? 0 : 1 + staticLog2(x / 2);
	}

	/**
		Function calculates a frequency weighting function using a Nth order highpass and lowpass combination. The weight is the magnitude response of the combined filters.
		@param f			frequency to calculate the weight at
		@param fs			the sample rate
		@param fLP			lowpass frequency
		@param fHP			highpass frequency
		@param hpfOrder		the order of the highpass filters 
		@param lpfOrder		the order of the lowpass filters
		@return the weight or magnitude response for the given frequency
	*/
	float frequencyWeight(float f, float fs, float fHP, float fLP, unsigned int hpfOrder, unsigned int lpfOrder);

	
	/**
		A simple first order allpass filter implementation. With coeff c, filter response is
		H = (c + 1 z^(-1)) / (1 + c * z^(-1))
		@param T Datatype
	*/
	template <typename T>
	class AllpassFirstOrer
	{
	public:

		/**
			Sets the coeff
			@param lambda the coefficient
		*/
		void setCoeff(const T& lambda);

		/**
			Processes a single sample with the filter
			@param input the input sample
			@return the output sample
		*/
		T tick(const T input);

	private:
		T z1{ T(0) };
		T c{ T(0) };
	};

	template<typename T>
	inline void AllpassFirstOrer<T>::setCoeff(const T & lambda)
	{
		this->c = lambda;
	}

	template<typename T>
	inline T AllpassFirstOrer<T>::tick(const T input)
	{
		// implements the transposed direct form II
		auto out = c * input + z1;
		z1 = input - c * out;
		return out;
	}
};