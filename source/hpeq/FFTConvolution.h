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
	FFConvolution implements a latency heavy, brute force convolution engine using FFTs. 
	@param MaxSize maximum supported impulse response time, needs to be power of 2
	
	For a impuls response of size N = 2^n we
	- store input samples until we have N samples ready to go
	- zero pad to 2*N, perform FFT, per sample multiplication, IFFT of 2*N
	- we store the first half (N samples) in primary queue, adding old (N..2*N) samples from the secondary queue
	- we store the second half (N samples) in secondary queue for next FFT

	That means: all the latency, very badly balanced CPU load. 
*/
template<unsigned int MaxSize>
class FFTConvolution : public AConvolutionEngine
{
private:
	static const unsigned int MinOrder = 5;
	static const unsigned int MaxOrder = 1 + IRTools::staticLog2(MaxSize);

public:
	FFTConvolution();

	// Inherited via AConvolutionEngine
	virtual void process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples) override;
	virtual void onImpulseResponseUpdated() override;

private:

	void updateKernelFFT();

	/**
		Runs the FFT convolution and puts samples were they belong
	*/
	void performConvolution(); 

private:



	std::array<std::complex<float>, 2 * MaxSize> audioFFTs[2];

	unsigned int numQueuedSamples{ 0 };

	// used to write directly to output
	StaticQueue<float, MaxSize> outputQueue[2];

	// used to cache output latency tail and add to
	StaticQueue<float, MaxSize> secondaryOutputQueue[2];



	std::array<std::complex<float>, MaxSize> kernelFFTs[2];

	std::map<unsigned int, std::unique_ptr<AFourierTransform>> fftEngines; // contains fft fftEngines assigned to their orders

	unsigned int currentFFTOrder{ MinOrder }; 
	unsigned int currentFFTSize{ 0 };

};

template<unsigned int MaxSize>
inline FFTConvolution<MaxSize>::FFTConvolution()
{
	assert(isPowerOfTwo(MaxSize));


	for (int order = MinOrder; order <= MaxOrder; order++)
	{
		fftEngines[order] = std::unique_ptr<AFourierTransform>(AFourierTransformFactory::FourierTransform(order));
	}

	onImpulseResponseUpdated();
}

template<unsigned int MaxSize>
inline void FFTConvolution<MaxSize>::process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples)
{
	for (int i = 0; i < numSamples; i++)
	{
		// cache input in audio working buffer
		audioFFTs[0][this->numQueuedSamples]  = readL[i];
		audioFFTs[1][this->numQueuedSamples] = readR[i];
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
inline void FFTConvolution<MaxSize>::onImpulseResponseUpdated()
{
	unsigned int size = getIR()->getSize();
	size = IRTools::nextPow2(size);

	assert(size <= MaxSize);

	unsigned int requiredOrder = IRTools::staticLog2(size) + 1;

	unsigned int usedOrder = std::max(requiredOrder, MinOrder);
	assert(requiredOrder <= MaxOrder);

	currentFFTOrder = usedOrder;

	currentFFTSize =  std::pow(2, currentFFTOrder - 1);

	/*
		Some background:
		We have a IR of size N=2^n, lets say 16. We want to use the next possible FFT order, so 32. A convolution
		will spit out (N+M-1) samples, so we can collect 17 samples (2^n+1) befor we need to run the FFT. For simplicity, we can 
		ignore that last sample and just work with 2^n.
	*/

	numQueuedSamples = 0;
	outputQueue[0].clear();
	outputQueue[1].clear();
	secondaryOutputQueue[0].clear();
	secondaryOutputQueue[1].clear();

	// add queue to have enough output for 2^n samples
	outputQueue[0].push(0, currentFFTSize);
	outputQueue[1].push(0, currentFFTSize);
	secondaryOutputQueue[0].push(0, currentFFTSize);
	secondaryOutputQueue[1].push(0, currentFFTSize);

	updateKernelFFT();
}

template<unsigned int MaxSize>
inline void FFTConvolution<MaxSize>::updateKernelFFT()
{
	std::fill(kernelFFTs[0].begin(), kernelFFTs[0].end(), 0);
	std::fill(kernelFFTs[1].begin(), kernelFFTs[1].end(), 0);

	auto ir = getIR();

	for (int i = 0; i < ir->getSize(); i++)
	{
		kernelFFTs[0][i] = ir->getLeft()[i];
		kernelFFTs[1][i] = ir->getRight()[i];
	}

	fftEngines[currentFFTOrder]->performFFTInPlace(kernelFFTs[0].data());
	fftEngines[currentFFTOrder]->performFFTInPlace(kernelFFTs[1].data());
}

template<unsigned int MaxSize>
inline void FFTConvolution<MaxSize>::performConvolution()
{
	// zero pad input
	for (int i = currentFFTSize; i < (2 * currentFFTSize); i++)
	{
		audioFFTs[0][i] = audioFFTs[1][i] = 0;
	}

	// to frequency domain
	fftEngines[currentFFTOrder]->performFFTInPlace(audioFFTs[0].data());
	fftEngines[currentFFTOrder]->performFFTInPlace(audioFFTs[1].data());

	// per sample multiplication
	for (int i = 0; i < (2 * currentFFTSize); i++)
	{
		audioFFTs[0][i]  *= kernelFFTs[0][i];
		audioFFTs[1][i] *= kernelFFTs[1][i];
	}

	// back to time domain
	fftEngines[currentFFTOrder]->performIFFTInPlace(audioFFTs[0].data());
	fftEngines[currentFFTOrder]->performIFFTInPlace(audioFFTs[1].data());


	// cache the first half of the samples directly to the output
	// cache the second half to the queue for later use
	for (int i = 0; i < currentFFTSize; i++)
	{
		// write first half and secondary queue to output queue
		outputQueue[0].push(audioFFTs[0][i].real() + secondaryOutputQueue[0].pull());
		outputQueue[1].push(audioFFTs[1][i].real() + secondaryOutputQueue[1].pull());

		// write second half to secondary queue for later use
		secondaryOutputQueue[0].push(audioFFTs[0][i + currentFFTSize].real());
		secondaryOutputQueue[1].push(audioFFTs[1][i + currentFFTSize].real());
	}

	numQueuedSamples = 0;
}
