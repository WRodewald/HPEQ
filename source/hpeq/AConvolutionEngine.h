#pragma once 

#include "ImpulseResponse.h"

/**
	Class presents an interface for convolution engines
*/
class AConvolutionEngine
{
public:

	virtual ~AConvolutionEngine() = default;

	/**
		Performs the convolution block-wise. 

	*/
	virtual void process(const float *readL, const float *readR,  float *writeL, float *writeR, unsigned int numSamples) = 0;
	
	/**
		Sets the impulse response pointer. if pointer is nullptr, internally switches to dirac impulse response.
	*/
	inline void setImpulseResponse( const ImpulseResponse *impulseResponse);

	/**
		Called during setImpulseResponse to prepare the convolution engine for a new impulse response.
		Function is called in Audio Thread.
	*/
	virtual void onImpulseResponseUpdated() = 0;

protected:
	
	/**
		Returns a pointer to the current impulse response. Never returns nullptr.
	*/
	inline const ImpulseResponse * getIR();


private:
	const ImpulseResponse dirac;
	const ImpulseResponse *impulseResponse{ &dirac }; 
};



inline void AConvolutionEngine::setImpulseResponse(const ImpulseResponse *impulseResponse)
{
	this->impulseResponse = impulseResponse;
	if (this->impulseResponse == nullptr) this->impulseResponse = &dirac;
	onImpulseResponseUpdated();
}

inline const ImpulseResponse * AConvolutionEngine::getIR()
{
	return impulseResponse;
}
