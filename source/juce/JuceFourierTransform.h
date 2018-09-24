#pragma once

#include "../hpeq/AFourierTransform.h"
#include "JuceHeader.h"

#include <vector>

/**
	A AFourierTransform implementation based on juce's FFT class.
*/
class JuceFourierTransform : public AFourierTransform
{
public:

	JuceFourierTransform(unsigned int order);
	~JuceFourierTransform() = default;

	// Inherited via AFourierTransform
	virtual void performFFT(std::complex<float>* in, std::complex<float>* out) override;
	virtual void performIFFT(std::complex<float>* in, std::complex<float>* out) override;

	// Inherited via AFourierTransform
	virtual void performFFTInPlace(std::complex<float>* buffer) override;
	virtual void performIFFTInPlace(std::complex<float>* buffer) override;

private:

	juce::dsp::FFT fft;

	std::vector<juce::dsp::Complex<float>> bufferIn;
	std::vector<juce::dsp::Complex<float>> bufferOut;

};