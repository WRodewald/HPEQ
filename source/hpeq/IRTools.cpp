
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include "IRTools.h"
#include "AFourierTransformFactory.h"



void IRTools::makeMono(ImpulseResponse & ir)
{
	auto left  = ir.getLeft();
	auto right = ir.getRight();

	for (int i = 0; i < ir.getSize(); i++)
	{
		left[i] = right[i] = 0.5 * (left[i] + right[i]);
	}
}

void IRTools::octaveSmooth(ImpulseResponse & ir, float width, float fs)
{
	width = std::max(width, 0.f);

	assert(isPow2(ir.getSize()));
	if (ir.getSize() < 2) return;

	unsigned int size = ir.getSize();

	auto transform = AFourierTransformFactory::FourierTransform(std::log2(size));


	for (int c = 0; c < 2; c++)
	{
		std::vector<std::complex<float>> buffer;
		std::vector<std::complex<float>> bufferY;
		auto x = (c == 0) ? ir.getLeft() : ir.getRight();

		for (int i = 0; i < size; i++)
		{
			buffer.push_back(x[i]);
			bufferY.push_back(0);
		}

		// FFT
		transform->performFFTInPlace(buffer.data());

		// calculate wheighted average
		float xSum = 0;
		float wSum = 0;

		int iNyquist = buffer.size() / 2;
		for (int i = 0; i <= iNyquist; i++)
		{
			float f = fs * static_cast<float>(i) / static_cast<float>(buffer.size());

			float fMax = f * std::pow(2., 0.5 * width);
			float fMin = f / std::pow(2., 0.5 * width);


			int iMin = std::ceil(buffer.size()  * (fMin / fs));
			int iMax = iMin + std::floor(buffer.size() * (fMax - fMin)/fs);

			float sum = 0;

			for (int k = iMin; k <= iMax; k++)
			{
				float fBin = fs * static_cast<float>(k) / static_cast<float>(buffer.size());

				int   kLim = std::min(std::max(k, 0), iNyquist);

				//float weight = ((fMax - fMin) <= 0.) ? 1. : 0.5 * (1. - std::cos(2. * M_PI*(fBin - fMin) / (fMax - fMin)));

				sum += std::abs(buffer[kLim]);
			}

			sum /= static_cast<float>((iMax - iMin + 1));

			bufferY[i] = std::polar(sum, std::arg(buffer[i]));

			if ((i != 0) && (i != iNyquist))
			{
				bufferY[buffer.size() - i] = std::polar(sum, -std::arg(buffer[i]));
			}
		}
		
		// iFFT
		transform->performIFFTInPlace(bufferY.data());

		for (int i = 0; i < size; i++)
		{
			x[i] = std::real(bufferY[i]);
		}
	}
	delete transform;
}

void IRTools::normalize(ImpulseResponse & ir, float fs)
{
	assert(isPow2(ir.getSize()));
	if (ir.getSize() < 2) return;

	unsigned int size = ir.getSize();

	auto transform = AFourierTransformFactory::FourierTransform(std::log2(size));

	float avg = 0;
	
	for (int c = 0; c < 2; c++)
	{
		std::vector<std::complex<float>> buffer;
		auto x = (c == 0) ? ir.getLeft() : ir.getRight();

		for (int i = 0; i < size; i++)
		{
			buffer.push_back(x[i]);
		}

		// FFT
		transform->performFFTInPlace(buffer.data());

		// calculate wheighted average
		float xSum = 0;
		float wSum = 0;
		for (int i = 0; i < buffer.size(); i++)
		{
			float f = fs * static_cast<float>(i) / static_cast<float>(buffer.size());
			float w = frequencyWeight(f, fs, 50, 20000, 2);

			auto & bin = buffer[i];

			xSum += std::abs(bin) * w;
			wSum += w;
		}

		avg += xSum / wSum;

	}

	avg *= 0.5;

	for (int c = 0; c < 2; c++)
	{
		std::vector<std::complex<float>> buffer;
		auto x = (c == 0) ? ir.getLeft() : ir.getRight();

		for (int i = 0; i < size; i++)
		{
			buffer.push_back(x[i]);
		}

		// FFT
		transform->performFFTInPlace(buffer.data());

		// calculate wheighted average
		float xSum = 0;
		float wSum = 0;
		for (int i = 0; i < buffer.size(); i++)
		{
			buffer[i] /= avg;
		}
		
		transform->performIFFTInPlace(buffer.data());

		for (int i = 0; i < buffer.size(); i++)
		{
			x[i] = buffer[i].real();
		}

	}

	delete transform;
}


void IRTools::fadeOut(ImpulseResponse & ir, float fs)
{
	assert(isPow2(ir.getSize()));
	if (ir.getSize() < 2) return;

	unsigned int size = ir.getSize();

	auto transform = AFourierTransformFactory::FourierTransform(std::log2(size));


	for (int c = 0; c < 2; c++)
	{
		std::vector<std::complex<float>> buffer;
		auto x = (c == 0) ? ir.getLeft() : ir.getRight();

		for (int i = 0; i < size; i++)
		{
			buffer.push_back(x[i]);
		}

		// FFT
		transform->performFFTInPlace(buffer.data());

		// calculate wheighted average
		float xSum = 0;
		float wSum = 0;
		for (int i = 0; i < buffer.size(); i++)
		{
			float f = fs * static_cast<float>(i) / static_cast<float>(buffer.size());
			float w = frequencyWeight(f, fs, 50, 20000, 2);

			auto & bin = buffer[i];

			xSum += std::abs(bin) * w;
			wSum += w;
		}

		float average = xSum / wSum;


		// fade out frequency response, normalize
		for (int i = 0; i < buffer.size(); i++)
		{
			float f = fs * static_cast<float>(i) / static_cast<float>(buffer.size());
			float w = frequencyWeight(f, fs, 50, 20000, 2);

			auto & bin = buffer[i];

			float mag = w * (std::abs(bin)) + (1 - w) * average;

			bin = std::polar(mag, std::arg(bin));
		}

		// iFFT
		transform->performIFFTInPlace(buffer.data());

		for (int i = 0; i < size; i++)
		{
			x[i] = std::real(buffer[i]);
		}
	}
	delete transform;
}

void IRTools::makeMinPhase(ImpulseResponse & ir)
{
	assert(isPow2(ir.getSize()));
	if (ir.getSize() < 2) return;

	unsigned int size   = ir.getSize();

	auto transform = AFourierTransformFactory::FourierTransform(std::log2(size));
	

	for (int c = 0; c < 2; c++)
	{
		std::vector<std::complex<float>> buffer;
		auto x = (c == 0) ? ir.getLeft() : ir.getRight();

		for (int i = 0; i < size; i++)
		{
			buffer.push_back(x[i]);
		}

		// FFT
		transform->performFFTInPlace(buffer.data());
		
		// remove phase, limit amplitude, take log
		for (auto & bin : buffer)
		{
			auto ampl = std::abs(bin);
			if (ampl <  0.000001) ampl = 0.000001; // argh
			bin = std::log(ampl);
		}

		// iFFT
		transform->performIFFTInPlace(buffer.data());
			
		// special sauce mask
		for (int i = 0; i < buffer.size(); i++)
		{
			auto gain = ((i == 0) ? 1 : ((i >= size / 2) ? 0 : 2));
			buffer[i] = std::real(buffer[i]) * gain;
		}

		// FFT
		transform->performFFTInPlace(buffer.data());

		// exp in FFT
		for (auto & bin : buffer)
		{
			bin = std::exp(bin);
		}

		// iFFT
		transform->performIFFTInPlace(buffer.data());
		
		for (int i = 0; i < size; i++)
		{
			x[i] = std::real(buffer[i]);
		}
	}
	delete transform;
}

ImpulseResponse IRTools::warp(const ImpulseResponse & ir, float lambda, unsigned int len)
{
	std::vector<float> out[2];

	assert((-1 <= lambda) && (lambda <= 1));
	
	for (auto c : { 0,1 })
	{
		auto in = ir.getVector(c);


		std::vector<float> temp;
		out[c].resize(len);
		temp.resize(len);
		temp[0] = 1;
		out[c][0] = in[0];

		for (int i = 1; i < len; i++)
		{
			AllpassFirstOrer<float> filter;
			filter.setCoeff(lambda);

			// filter temp buffer
			for (auto & bin : temp) bin = filter.tick(bin);

			// accumulate on output
			for (int k = 0; k < len; k++)
			{
				out[c][k] += in[i] * temp[k];
			}
		}
	}

	return ImpulseResponse(out[0], out[1], ir.getSampleRate());
}

void IRTools::invertMagResponse(ImpulseResponse & ir)
{
	assert(isPow2(ir.getSize()));
	if (ir.getSize() < 2) return;

	unsigned int size = ir.getSize();

	auto transform = AFourierTransformFactory::FourierTransform(std::log2(size));


	for (int c = 0; c < 2; c++)
	{
		std::vector<std::complex<float>> buffer;
		auto x = (c == 0) ? ir.getLeft() : ir.getRight();

		for (int i = 0; i < size; i++)
		{
			buffer.push_back(x[i]);
		}

		// FFT
		transform->performFFTInPlace(buffer.data());

		// remove phase, limit amplitude, take log
		for (auto & bin : buffer)
		{
			auto ampl = std::abs(bin);
			if (ampl <  0.0001) ampl = 0.0001; // argh
			bin /= (ampl * ampl);
		}

		// iFFT
		transform->performIFFTInPlace(buffer.data());
		
		for (int i = 0; i < size; i++)
		{
			x[i] = std::real(buffer[i]);
		}
	}
	delete transform;
}

void IRTools::zeroPadToPow2(ImpulseResponse & ir)
{
	if (!isPow2(ir.getSize()))
	{
		auto newSize = nextPow2(ir.getSize());
		
		std::vector<float> left, right;
		
		std::fill(left.begin(),  left.end(), 0);
		std::fill(right.begin(), right.end(), 0);

		for (int i = 0; i < ir.getSize(); i++)
		{
			left[i] = ir.getLeft()[i];
			right[i] = ir.getRight()[i];
		}

		ir = ImpulseResponse(left, right, ir.getSampleRate());
	}
}

unsigned int IRTools::nextPow2(unsigned int x)
{	
	return (x <= 1) ? 1 : (2 * nextPow2((x + 1) >> 1));
}

bool IRTools::isPow2(unsigned int x)
{
	return x == nextPow2(x);
}

float IRTools::frequencyWeight(float f, float fs, float fHP, float fLP, unsigned int order)
{
	std::complex<double> z = std::polar(1., 2.f * M_PI * f / fs);

	// filter coefficients
	double bLP = 1. - std::exp(-2. * M_PI * fLP / fs);
	double bHP = 1. - std::exp(-2. * M_PI * fHP / fs);

	// filter responses
	auto zI = std::pow(z, -1);
	auto hLP = (0.5 + 0.5 * zI) * bLP  / (1. + (bLP - 1) * zI); 
	auto hHP = 1. - bHP / (1. + (bHP - 1) * zI);
	
	// combined response
	return std::pow(std::abs(hHP * hLP), order);
}
