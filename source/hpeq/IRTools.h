#pragma once


#include "ImpulseResponse.h"




/**

	A collection of static functions for impulse response modifications
*/
namespace IRTools
{

	/**
		Function removes stereo information from the impulse response
		@param ir the impulse response
	*/
	void makeMono(ImpulseResponse & ir);

	/**
	Function smoothes the frequency response with a octave-width band
		@param ir		the impulse response
		@param width	kernel width in octaves
		@param fs		the sample rate
	*/
	void octaveSmooth(ImpulseResponse & ir, float width, float fs);

	/**
		Function normalizes the frequency response overall amplitude
		Only works with impulse responses with size 2^n.
		@param ir the impulse response
		@param fs the sample rate
	*/
	void normalize(ImpulseResponse & ir, float fs);

	/**
	Function fades out the frequency response at very low and high frequencies.
	Only works with impulse responses with size 2^n.
	@param ir the impulse response
	@param fs the sample rate
	*/
	void fadeOut(ImpulseResponse & ir, float fs);


	/**
		Function converts the impulse respons to a minimum phase system. 
		Only works with impulse responses with size 2^n.
		@param ir the impulse response
	*/
	void makeMinPhase(ImpulseResponse & ir);

	/**
		Function creates a warped version of the ImpulseResponse ir with warping coeff ir and length len
		@param ir the original impulse response
		@param lambda allpass coefficient lambda
		@param len length of the output impulse response
		@return the warped impulse response
	*/
	ImpulseResponse warp(const ImpulseResponse & ir, float lambda, unsigned int len);


	/**
		Function inverts the amplitude magnitude response. 
		Only works with impulse responses with size 2^n.
		@param ir the impulse response
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
		Function returns true if the size of the impulse response is a power of 2
	*/
	bool isPow2(unsigned int x);

	/**
		Function returns the log of 2 for x = 2^n.
	*/
	constexpr unsigned int staticLog2(unsigned int x)
	{
		return (x <= 1) ? 0 : 1 + staticLog2(x / 2);
	}

	/**
		Function calculates a frequency wheight in terms of a Nth order 2nd-order band pass (1st order high pass / lowpass in sereis)
		@param f frequency to calculate the weight at
		@param fs the sample rate
		@param fLP lowpass frequency
		@param fHP highpass frequency
		@param order the order of the filters H = (HLP^order * HHP^order)
	*/
	float frequencyWeight(float f, float fs, float fHP, float fLP, unsigned int order);
	
	/*
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