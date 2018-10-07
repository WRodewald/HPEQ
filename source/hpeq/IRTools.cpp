
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include "IRTools.h"
#include "AFourierTransformFactory.h"



ImpulseResponse IRTools::resample(const ImpulseResponse & ir, float targetSampleRate, unsigned int windowWidth)
{
	if (targetSampleRate == ir.getSampleRate()) return ir;
	auto  fs = ir.getSampleRate();
	auto  tFs = targetSampleRate;
	float ratio = tFs / fs;

	unsigned int lengthSource = ir.getSize();
	unsigned int lengthTarget = std::round(ratio* lengthSource);

	auto normalizedSampleRate = std::min(ratio, 1.f);

	std::vector<float> buffers[2];

	windowWidth = std::max(2U, windowWidth + (windowWidth % 1));
	bool useWindowed = lengthSource > windowWidth;

	for (auto c : { 0,1 })
	{
		auto & buffer = buffers[c];
		buffer.resize(lengthTarget);
		std::fill(buffer.begin(), buffer.end(), 0);
		const auto &source = ir.getVector(c);
		
		for (int i = 0; i < buffer.size(); i++)
		{
			// aligned position of source
			float kFrac = static_cast<float>(i) / ratio;
			
			float kMinFrac = useWindowed ? kFrac - 0.5f*windowWidth :  0;
			float kMaxFrac = useWindowed ? kFrac + 0.5f*windowWidth : lengthSource-1;

			int kMin = std::max(static_cast<int>(std::floor(kMinFrac)), 0);
			int kMax = std::min(static_cast<int>(std::ceil(kMaxFrac)), static_cast<int>(lengthSource - 1));
						
			for (int k = kMin; k <= kMax; k++)
			{
				float kRel = kFrac - k;
				
				// sinc weight
				float w = (std::abs(kRel) <= 0.000001) ? 1.f :  std::sin(M_PI * kRel * normalizedSampleRate) / (M_PI*kRel * normalizedSampleRate);

				// hann window
				if(useWindowed) w *= 0.5 * (1. - std::cos(2. * M_PI * (k-kMin) / (kMax-kMin+0.0001)));
				
				buffer[i] += w * source[k];
			}
		}
	}

	return ImpulseResponse(buffers[0], buffers[1], targetSampleRate);
}

void IRTools::makeMono(ImpulseResponse & ir)
{
	auto left  = ir.getLeft();
	auto right = ir.getRight();

	for (int i = 0; i < ir.getSize(); i++)
	{
		left[i] = right[i] = 0.5 * (left[i] + right[i]);
	}
}

ImpulseResponse  IRTools::truncate(const ImpulseResponse & ir, float dbThreshold, bool keepPow2)
{	

	float magThreshold = std::pow(10, dbThreshold / 20.f);

	float fullEnergy[2] = { 0,0 };
	for (int i = 0; i < ir.getSize(); i++)
	{
		float l = ir.getLeft()[i];
		float r = ir.getRight()[i];

		fullEnergy[0] += (l*l);
		fullEnergy[1] += (r*r);
	}
	
	float sqrSum[2] = { 0,0 };
	unsigned int lastBin;
	for (int i = ir.getSize()-1; i >= 0; i--)
	{
		lastBin = i;

		float l = ir.getLeft()[i];
		float r = ir.getRight()[i];

		sqrSum[0] += (l*l);
		sqrSum[1] += (r*r);

		auto len = ir.getSize() - i;
		float relRms = std::sqrt(std::max(sqrSum[0] / fullEnergy[0], sqrSum[1] / fullEnergy[1]));

		if(relRms >= magThreshold)
		{
			break;
		}
	}
	
	auto newLen = lastBin + 1;
	
	if (keepPow2)
	{
		newLen = nextPow2(newLen);	
	}
	
	if (newLen == ir.getSize()) return ir;

	auto left  = ir.getLeftVector();
	auto right = ir.getRightVector();

	left.resize(newLen);
	right.resize(newLen);

	return ImpulseResponse(left, right, ir.getSampleRate());
}

void IRTools::octaveSmooth(ImpulseResponse & ir, float width)
{
	auto fs = ir.getSampleRate();
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

void IRTools::normalize(ImpulseResponse & ir)
{
	assert(isPow2(ir.getSize()));
	if (ir.getSize() < 2) return;

	unsigned int size = ir.getSize();

	bool useWeighting = (ir.getSize() > 16);

	auto fs = ir.getSampleRate();

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
			float w = useWeighting ? frequencyWeight(f, fs, 50, 20000, 2, 2) : 1;

			auto & bin = buffer[i];

			xSum += std::abs(bin) * w;
			wSum += w;
		}				
		avg += useWeighting ? xSum / wSum : xSum;

	}

	avg *= 0.5;
	avg = std::max(avg, 0.0001f);

	for (int c = 0; c < 2; c++)
	{
		auto x = (c == 0) ? ir.getLeft() : ir.getRight();

		for (int i = 0; i < size; i++)
		{
			x[i] /= avg;
		}
	}

	delete transform;
}


void IRTools::fadeOut(ImpulseResponse & ir, float fHP, float fLP, unsigned int hpfOrder, unsigned int lpfOrder)
{
	assert(isPow2(ir.getSize()));
	if (ir.getSize() < 2) return;
	
	auto fs = ir.getSampleRate();

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
			float w = frequencyWeight(f, fs, fHP, fLP, hpfOrder, lpfOrder);

			auto & bin = buffer[i];

			xSum += std::abs(bin) * w;
			wSum += w;
		}

		float average = xSum / wSum;


		// fade out frequency response, normalize
		for (int i = 0; i < buffer.size(); i++)
		{
			float f = fs * static_cast<float>(i) / static_cast<float>(buffer.size());
			float w = frequencyWeight(f, fs, fHP, fLP, hpfOrder, lpfOrder);

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
	
	auto minAmp = std::exp(-60);

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
			if (ampl <  minAmp) ampl = minAmp; // argh
			bin = std::log(ampl);
		}

		// iFFT
		transform->performIFFTInPlace(buffer.data());
		
		bool isEven = (size % 2) == 0;

		// special sauce mask
		buffer[0] = std::real(buffer[0]);
		for (int i = 1; i < buffer.size(); i++)
		{
			auto gain = (isEven && (i == 0.5*size)) ?  1 :  (((i >= size / 2) ? 0 : 2));
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
	if (lambda == 0) return ir;
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

		left.resize(newSize);
		right.resize(newSize);
		
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

float IRTools::frequencyWeight(float f, float fs, float fHP, float fLP, unsigned int hpfOrder, unsigned int lpfOrder)
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
	return std::pow(std::abs(hHP), hpfOrder) * std::pow(std::abs(hLP), lpfOrder);
}
