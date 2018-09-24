#include "JuceFourierTransform.h"

using namespace juce::dsp;

JuceFourierTransform::JuceFourierTransform(unsigned int order) : AFourierTransform(order), fft(order)
{
	bufferIn.reserve(getSize());
	bufferOut.reserve(getSize());

	for (int i = 0; i < getSize(); i++)
	{
		bufferIn.push_back(0);
		bufferOut.push_back(0);
	}
}

void JuceFourierTransform::performFFT(std::complex<float>* in, std::complex<float>* out)
{
	for (int i = 0; i < getSize(); i++)
	{
		bufferIn[i] = in[i];
	}

	fft.perform(bufferIn.data(), bufferOut.data(), false);

	for (int i = 0; i < getSize(); i++)
	{
		out[i] = bufferOut[i];
	}
}

void JuceFourierTransform::performIFFT(std::complex<float>* in, std::complex<float>* out)
{
	for (int i = 0; i < getSize(); i++)
	{
		bufferIn[i] = in[i];
	}

	fft.perform(bufferIn.data(), bufferOut.data(), true);

	for (int i = 0; i < getSize(); i++)
	{
		out[i] = bufferOut[i];
	}
}

void JuceFourierTransform::performFFTInPlace(std::complex<float>* buffer)
{
	for (int i = 0; i < getSize(); i++)
	{
		bufferIn[i] = buffer[i];
	}

	fft.perform(bufferIn.data(), bufferOut.data(), false);

	for (int i = 0; i < getSize(); i++)
	{
		buffer[i] = bufferOut[i];
	}
}

void JuceFourierTransform::performIFFTInPlace(std::complex<float>* buffer)
{
	for (int i = 0; i < getSize(); i++)
	{
		bufferIn[i] = buffer[i];
	}

	fft.perform(bufferIn.data(), bufferOut.data(), true);

	for (int i = 0; i < getSize(); i++)
	{
		buffer[i] = bufferOut[i];
	}
}

