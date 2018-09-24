#pragma once


#include "AConvolutionEngine.h"
#include "StaticRingBuffer.h"


/**
	A time domain convlolution engine with a static MaxSize. MaxSize has to be a power of 2.

*/
template<unsigned int MaxSize>
class TimeDomainConvolution : public AConvolutionEngine
{
public:
	virtual void process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples) override;
		
protected:
	virtual void onImpulseResponseUpdated() override;
	
private:

	// ToDo: replace left and right with vector or at least with a struct containing both l and r
	// No need to have two seperate buffers

	StaticRingBuffer<float, MaxSize> bufferL;
	StaticRingBuffer<float, MaxSize> bufferR;
};


template<unsigned int MaxSize>
void TimeDomainConvolution<MaxSize>::process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples)
{
	// cache some variables for readability 
	auto ir = getIR();
	auto irSize = std::min(ir->getSize(), bufferL.getLength()); // double check size to prevent access violation 
	auto irBufferL = ir->getLeft();
	auto irBufferR = ir->getRight();


	for (int i = 0; i < numSamples; i++)
	{
		float l = readL[i];
		float r = readR[i];

		// add the IR multiplied by input sample in the ringbuffer
		for (int k = 0; k < irSize; k++)
		{
			unsigned int writePos = irSize - k - 1;
			bufferL[writePos] += l * irBufferL[k];
			bufferR[writePos] += r * irBufferR[k];
		}

		// read out current sample and increment
		writeL[i] = bufferL.tick(0);
		writeR[i] = bufferR.tick(0);

	}
}

template<unsigned int MaxSize>
void TimeDomainConvolution<MaxSize>::onImpulseResponseUpdated()
{
	auto ir = getIR();

	// limit the size we use to what we can put in the buffer
	unsigned int clippedIRSize = std::min(ir->getSize(), static_cast<unsigned int>(MaxSize));

	// resize the buffer and shrink to fit
	this->bufferL.setLengthAndResize(clippedIRSize);
	this->bufferR.setLengthAndResize(clippedIRSize);
}
