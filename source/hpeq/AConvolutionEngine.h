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
		@param readL pointer to the first sample of the left input buffers
		@param readR pointer to the first sample of the right input buffers
		@param writeL pointer to the first sample of the left output buffers
		@param writeR pointer to the first sample of the right output buffers
		@param numSamples the number of samples in the input  output buffers
	*/
	virtual void process(const float *readL, const float *readR,  float *writeL, float *writeR, unsigned int numSamples) = 0;
		
	/**
		Sets the impulse response. May not be called from audio thread.
	*/
	inline void setImpulseResponse(const ImpulseResponse &impulseResponse);
	
	/**
		Called when #setImpulseResponse was called
	*/
	inline virtual void onImpulseResponseUpdate() {};

	const ImpulseResponse * getImpulseResponse() const;

private:
	ImpulseResponse impulseResponse;
};

void AConvolutionEngine::setImpulseResponse(const ImpulseResponse &impulseResponse)
{
	this->impulseResponse = impulseResponse;
	onImpulseResponseUpdate();
}

inline const ImpulseResponse * AConvolutionEngine::getImpulseResponse() const
{
	return &impulseResponse;
}
