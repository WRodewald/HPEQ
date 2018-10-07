#pragma once


#include "ASyncedConvolutionEngine.h"
#include "StaticRingBuffer.h"


/**
	A time domain convlolution engine with a static MaxSize. MaxSize has to be a power of 2.
*/
template<unsigned int MaxSize>
class TimeDomainConvolution : public ASyncedConvolutionEngine<ImpulseResponse>
{
public:
	virtual void process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples) override;
	
protected:
	// Inherited via ASyncedConvolutionEngine
	virtual ImpulseResponse preProcess(const ImpulseResponse & ir) override;

	virtual void onDataUpdate() override;

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
	updateData();

	auto ir = getData();
	auto irSize = std::min(ir->getSize(), bufferL.getLength()); // double check size to prevent access violation 
	auto irBufferL = ir->getLeft();
	auto irBufferR = ir->getRight();


	for (int i = 0; i < numSamples; i++)
	{
		float l = readL[i];
		float r = readR[i];

		// current sample
		writeL[i] =  bufferL[0] += l * irBufferL[0];
		writeR[i] =  bufferR[0] += r * irBufferR[0];

		// add the IR multiplied by input sample in the ringbuffer to future samples
		for (int k = 1; k < irSize; k++)
		{
			// writing to the futre, yea!
			bufferL[-k] += l * irBufferL[k];
			bufferR[-k] += r * irBufferR[k];
		}
		
		// flush current sample and increment
		bufferL[0] = bufferR[0] = 0;
		bufferR.increment();
		bufferL.increment();
	}
}

template<unsigned int MaxSize>
inline ImpulseResponse TimeDomainConvolution<MaxSize>::preProcess(const ImpulseResponse & ir)
{
	return ir;
}

template<unsigned int MaxSize>
void TimeDomainConvolution<MaxSize>::onDataUpdate()
{
	auto ir = getData();

	// limit the size we use to what we can put in the buffer
	unsigned int clippedIRSize = std::min(ir->getSize(), static_cast<unsigned int>(MaxSize));

	// resize the buffer and shrink to fit
	this->bufferL.setLengthAndResize(clippedIRSize);
	this->bufferR.setLengthAndResize(clippedIRSize);
}
