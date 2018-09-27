#pragma once

#include "AConvolutionEngine.h"
#include "AFourierTransformFactory.h"
#include "StaticQueue.h"
#include "IRTools.h"
#include <array>
#include <algorithm>
#include <memory>
#include <map>


/**
	FFTPartConvolution implements a simple partitioned FFT where the full impulse response is split in M partitions. 
	@param MaxSize maximum supported impulse response time, needs to be power of 2
*/
template<unsigned int MaxSize>
class FFTPartConvolution : public AConvolutionEngine
{
private:
	static const unsigned int MinOrder = 5;
	static const unsigned int MaxOrder = 1 + IRTools::staticLog2(MaxSize);

public:
	FFTPartConvolution();

	// Inherited via AConvolutionEngine
	virtual void process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples) override;
	virtual void onImpulseResponseUpdated() override;

	/*
		Sets the number of partitions used to split the impulse response with number P = 2^order
		@param order the order for the partitioning where the number of partition equals P = 2^order
	**/
	void setPartitioningOrder(unsigned int order);

private:

	/**
		Updates the fourier transformed IR partitions
	*/
	void updateKernelFFT();

	/**
		Runs the FFT convolution and puts samples were they belong
	*/
	void performConvolution(); 

private:


	// caches the input fft buffer befor it's
	std::array<std::complex<float>, 2 * MaxSize> audioInput[2];
		
	// the number of samples currently stored in audioInput
	unsigned int numQueuedSamples{ 0 };

	// caches one or multiple partition FFTs in this 
	std::array<std::complex<float>, 2 * MaxSize> partitionFFTCache[2];
	

	// used to write directly to output
	StaticQueue<float, MaxSize> outputQueue[2];

	// used to cache output latency tail and add to
	StaticQueue<float, MaxSize> secondaryOutputQueue[2];
	
	// fft's of our partitioned impulse response
	std::array<std::complex<float>, 2*MaxSize> kernelFFTs[2];

	std::map<unsigned int, std::unique_ptr<AFourierTransform>> fftEngines; // contains fft fftEngines assigned to their orders

	unsigned int currentFFTOrder{ MinOrder }; 

	// size of the partitions and fft
	unsigned int currentFFTSize{ 0 };

	// order of paritioning as requested by extern calls
	unsigned int requestedPartOrder{ 0 };

	// number of partitions currently used
	unsigned int numPartitions{ 1 };

	// index of the current partition;
	unsigned int currentPartition{ 0 };

};

template<unsigned int MaxSize>
inline FFTPartConvolution<MaxSize>::FFTPartConvolution()
{
	assert(isPowerOfTwo(MaxSize));
	
	for (int order = MinOrder; order <= MaxOrder; order++)
	{
		fftEngines[order] = std::unique_ptr<AFourierTransform>(AFourierTransformFactory::FourierTransform(order));
	}

	onImpulseResponseUpdated();
}

template<unsigned int MaxSize>
inline void FFTPartConvolution<MaxSize>::process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples)
{
	for (int i = 0; i < numSamples; i++)
	{
		// cache input in audio working buffer
		audioInput[0][this->numQueuedSamples] = readL[i];
		audioInput[1][this->numQueuedSamples] = readR[i];
		numQueuedSamples++;

		// perform fft if we have enough samples
		if (numQueuedSamples == currentFFTSize)
		{
			performConvolution();
		}

		// write from queue to output
		writeL[i] = outputQueue[0].pull();
		writeR[i] = outputQueue[1].pull();
	}
}


template<unsigned int MaxSize>
inline void FFTPartConvolution<MaxSize>::onImpulseResponseUpdated()
{
	unsigned int size = getIR()->getSize();
	size = IRTools::nextPow2(size);

	assert(size <= MaxSize);

	// now calculating the size of parititons
	unsigned int partSize = size >> requestedPartOrder;

	unsigned int requiredFFTOrder = IRTools::staticLog2(partSize) + 1;

	unsigned int usedOrder = std::max(requiredFFTOrder, MinOrder);
	assert(requiredFFTOrder <= MaxOrder);

	currentFFTOrder = usedOrder;

	currentFFTSize =  std::pow(2, currentFFTOrder - 1);

	numPartitions = std::max(size / currentFFTSize, static_cast<unsigned int>(1));

	/*
		Some background:
		We have a IR of size N=2^n, lets say 16. We want to use the next possible FFT order, so 32. A convolution
		will spit out (N+M-1) samples, so we can collect 17 samples (2^n+1) befor we need to run the FFT. For simplicity, we can 
		ignore that last sample and just work with 2^n.
	*/

	// clear buffers, prepare queues
	for (auto c : { 0,1 })
	{
		audioInput[c].fill(0);
		partitionFFTCache[c].fill(0);
		outputQueue[c].clear();
		outputQueue[c].push(0, currentFFTSize);

		secondaryOutputQueue[c].clear();
		secondaryOutputQueue[c].push(0, currentFFTSize);
	}

	numQueuedSamples = 0;
	currentPartition = 0;


	updateKernelFFT();
}

template<unsigned int MaxSize>
inline void FFTPartConvolution<MaxSize>::setPartitioningOrder(unsigned int order)
{
	this->requestedPartOrder = order;
	onImpulseResponseUpdated();
}

template<unsigned int MaxSize>
inline void FFTPartConvolution<MaxSize>::updateKernelFFT()
{
	/**
		Some notes: the partitions are split up evenly, with partition size P, the first partition in
		kernelFFTs is at index 0, second at P, third at 2*P and so on.
	*/

	// set order, just to be sure
	currentFFTOrder = IRTools::staticLog2(currentFFTSize) + 1;
	for (int c : {0, 1})
	{
		kernelFFTs[c].fill(0);

		auto buffer = getIR()->getChannel(c);

		for (int p = 0; p < numPartitions; p++)
		{
			unsigned int irBufferOffset  = p * currentFFTSize;
			unsigned int fftBufferOffset = 2 * irBufferOffset;

			unsigned int irMaxPos = std::min(getIR()->getSize() - irBufferOffset, currentFFTSize);
			for (int i = 0; i < irMaxPos; i++)
			{
				kernelFFTs[c][i + fftBufferOffset] = buffer[i + irBufferOffset];
			}
			
			fftEngines[currentFFTOrder]->performFFTInPlace(&kernelFFTs[c][fftBufferOffset]);
		}
	}
}

template<unsigned int MaxSize>
inline void FFTPartConvolution<MaxSize>::performConvolution()
{
	/*
		For every performConvolution call, we need to:
		- perform FFT of the new input
		- for every partition
			- complex multiplication
		- sum up all partiotions
		- ifft
		- feed queues

		Note on the partitionFFTCache: We handle it as a ring buffer, 

	*/
	

	// zero pad input
	for (int i = currentFFTSize; i < (2 * currentFFTSize); i++)
	{
		audioInput[0][i] = audioInput[1][i] = 0;
	}

	auto currentPartitionChaceOffset = currentPartition * (2 * currentFFTSize);

	// to frequency domain
	fftEngines[currentFFTOrder]->performFFT(audioInput[0].data(), &partitionFFTCache[0][currentPartitionChaceOffset]);
	fftEngines[currentFFTOrder]->performFFT(audioInput[1].data(), &partitionFFTCache[1][currentPartitionChaceOffset]);

	// first partition out of loop so that we don't need to flush the buffer first
	for (int i = 0; i < (2*currentFFTSize); i++)
	{
		audioInput[0][i] = partitionFFTCache[0][currentPartitionChaceOffset + i] * kernelFFTs[0][i];
		audioInput[1][i] = partitionFFTCache[1][currentPartitionChaceOffset + i] * kernelFFTs[1][i];
	}

	// per parition
	for (int p = 1; p < numPartitions; p++)
	{
		auto kernelOffset = 2 * currentFFTSize * p;
		auto cacheOffset  = 2 * currentFFTSize * ((currentPartition + numPartitions - p) % numPartitions);
		for (int i = 0; i < (2 * currentFFTSize); i++)
		{
			audioInput[0][i] += partitionFFTCache[0][cacheOffset + i] * kernelFFTs[0][kernelOffset + i];
			audioInput[1][i] += partitionFFTCache[1][cacheOffset + i] * kernelFFTs[1][kernelOffset + i];
		}
	}

	// back to time domain
	fftEngines[currentFFTOrder]->performIFFTInPlace(audioInput[0].data());
	fftEngines[currentFFTOrder]->performIFFTInPlace(audioInput[1].data());


	// cache the first half of the samples directly to the output
	// cache the second half to the queue for later use
	for (int i = 0; i < currentFFTSize; i++)
	{
		// write first half and secondary queue to output queue
		outputQueue[0].push(audioInput[0][i].real() + secondaryOutputQueue[0].pull());
		outputQueue[1].push(audioInput[1][i].real() + secondaryOutputQueue[1].pull());

		// write second half to secondary queue for later use
		secondaryOutputQueue[0].push(audioInput[0][i + currentFFTSize].real());
		secondaryOutputQueue[1].push(audioInput[1][i + currentFFTSize].real());
	}

	// increment artition
	currentPartition = (currentPartition + 1) % numPartitions;
	numQueuedSamples = 0;
}
