#pragma once

#include "AConvolutionEngine.h"
#include "AFourierTransformFactory.h"
#include "StaticQueue.h"
#include "IRTools.h"
#include <array>
#include <algorithm>
#include <memory>
#include <map>

constexpr unsigned int staticLog2(unsigned int x)
{
	return (x <= 1) ? 0 : 1 + staticLog2(x / 2);
}

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
	static const unsigned int MaxOrder = 1 + staticLog2(MaxSize);

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



	std::array<std::complex<float>, 2 * MaxSize> audioFFTLeft;
	std::array<std::complex<float>, 2 * MaxSize> audioFFTRight;

	unsigned int numQueuedSamples{ 0 };

	// used to write directly to output
	StaticQueue<float, MaxSize> outputQueueLeft;
	StaticQueue<float, MaxSize> outputQueueRight;

	// used to cache output latency tail and add to
	StaticQueue<float, MaxSize> secondaryOutputQueueLeft;
	StaticQueue<float, MaxSize> secondaryOutputQueueRight;



	std::array<std::complex<float>, MaxSize> kernelFFTLeft;
	std::array<std::complex<float>, MaxSize> kernelFFTRight;

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
		audioFFTLeft[this->numQueuedSamples]  = readL[i];
		audioFFTRight[this->numQueuedSamples] = readR[i];
		numQueuedSamples++;

		// perform fft if we have enough samples
		if (numQueuedSamples == currentFFTSize)
		{
			performConvolution();
		}

		// write from queue to output
		writeL[i] = outputQueueLeft.pull();
		writeR[i] = outputQueueRight.pull();
	}
}


template<unsigned int MaxSize>
inline void FFTConvolution<MaxSize>::onImpulseResponseUpdated()
{
	unsigned int size = getIR()->getSize();
	size = IRTools::nextPow2(size);

	assert(size <= MaxSize);

	unsigned int requiredOrder = staticLog2(size) + 1;

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
	outputQueueLeft.clear();
	outputQueueRight.clear();
	secondaryOutputQueueLeft.clear();
	secondaryOutputQueueRight.clear();

	// add queue to have enough output for 2^n samples
	outputQueueLeft.push(0, currentFFTSize);
	outputQueueRight.push(0, currentFFTSize);
	secondaryOutputQueueLeft.push(0, currentFFTSize);
	secondaryOutputQueueRight.push(0, currentFFTSize);

	updateKernelFFT();
}

template<unsigned int MaxSize>
inline void FFTConvolution<MaxSize>::updateKernelFFT()
{
	std::fill(kernelFFTLeft.begin(), kernelFFTLeft.end(), 0);
	std::fill(kernelFFTRight.begin(), kernelFFTRight.end(), 0);

	auto ir = getIR();

	for (int i = 0; i < ir->getSize(); i++)
	{
		kernelFFTLeft[i] = ir->getLeft()[i];
		kernelFFTRight[i] = ir->getRight()[i];
	}

	fftEngines[currentFFTOrder]->performFFTInPlace(kernelFFTLeft.data());
	fftEngines[currentFFTOrder]->performFFTInPlace(kernelFFTRight.data());
}

template<unsigned int MaxSize>
inline void FFTConvolution<MaxSize>::performConvolution()
{
	// zero pad input
	for (int i = currentFFTSize; i < (2 * currentFFTSize); i++)
	{
		audioFFTLeft[i] = audioFFTRight[i] = 0;
	}

	// to frequency domain
	fftEngines[currentFFTOrder]->performFFTInPlace(audioFFTLeft.data());
	fftEngines[currentFFTOrder]->performFFTInPlace(audioFFTRight.data());

	// per sample multiplication
	for (int i = 0; i < (2 * currentFFTSize); i++)
	{
		audioFFTLeft[i]  *= kernelFFTLeft[i];
		audioFFTRight[i] *= kernelFFTRight[i];
	}

	// back to time domain
	fftEngines[currentFFTOrder]->performIFFTInPlace(audioFFTLeft.data());
	fftEngines[currentFFTOrder]->performIFFTInPlace(audioFFTRight.data());


	// cache the first half of the samples directly to the output
	// cache the second half to the queue for later use
	for (int i = 0; i < currentFFTSize; i++)
	{
		// write first half and secondary queue to output queue
		outputQueueLeft.push(audioFFTLeft[i].real() + secondaryOutputQueueLeft.pull());
		outputQueueRight.push(audioFFTRight[i].real() + secondaryOutputQueueRight.pull());

		// write second half to secondary queue for later use
		secondaryOutputQueueLeft.push(audioFFTLeft[i + currentFFTSize].real());
		secondaryOutputQueueRight.push(audioFFTRight[i + currentFFTSize].real());
	}

	numQueuedSamples = 0;
}
