/*
  ==============================================================================

    ImpulseResponseViewComponent.cpp
    Created: 24 Sep 2018 1:19:17pm
    Author:  WRodewald

  ==============================================================================
*/

#include "../../JuceLibraryCode/JuceHeader.h"
#include "ImpulseResponseViewComponent.h"
#include "../hpeq/AFourierTransformFactory.h"

#include "../hpeq/IRTools.h"

//==============================================================================
ImpulseResponseViewComponent::ImpulseResponseViewComponent()
{
}

ImpulseResponseViewComponent::~ImpulseResponseViewComponent()
{
}

void ImpulseResponseViewComponent::setImpulseResponse(ImpulseResponse ir)
{
	this->ir = ir;
	repaint();
}

void ImpulseResponseViewComponent::paint (Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background

	int frameHeight = getHeight() * 0.5 - 1;

	juce::Rectangle<int> irBounds{ 0,0,getWidth(), frameHeight };
	juce::Rectangle<int> specBounds{ 0, getHeight() - frameHeight, getWidth(), frameHeight };


	painSpectrum(g, specBounds);
	paintImpulseResponse(g, irBounds);

	g.setColour(Colours::grey);
	g.drawRect(irBounds);
	g.drawRect(specBounds);

}

void ImpulseResponseViewComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
	repaint();

}

void ImpulseResponseViewComponent::painSpectrum(Graphics & g, juce::Rectangle<int> bounds)
{
	if (ir.getSize() < 2) return;
	auto irSize = ir.getSize();
	auto fftSize = IRTools::nextPow2(irSize);
	
	auto transform = AFourierTransformFactory::FourierTransform(std::log2(fftSize));


	auto dBRange = getFrequencyDomainDBScale(ir);

	auto dBMax = dBRange.second;
	auto dBMin = dBRange.first;

	auto fMin = 20;
	auto fMax = 22050;


	auto f2X = [=](float f)
	{
		auto relative = std::log2(f / fMin) / (log2(fMax / fMin));
		return bounds.getX() + 1 + relative * (bounds.getWidth() - 2);
	};

	auto dB2Y = [=](float dB)
	{
		auto relative = (dB - dBMin) / (dBMax - dBMin);
		return bounds.getY() + 1 + (1 - relative) * (bounds.getHeight() - 2);
	};
	
	auto mag2db = [](float mag)
	{
		mag = (mag < 0.000001) ? 0.000001 : mag;
		return 20 * std::log10(mag);
	};

	g.setColour(Colours::darkgrey);

	// vertical grid
	for (int db = std::round(dBMin/12)*12; db <= dBMax; db+= 12)
	{
		g.setColour(Colours::darkgrey);
		auto p1 = Point<float>(f2X(fMin), dB2Y(db));
		auto p2 = Point<float>(f2X(fMax), dB2Y(db));
		g.drawLine(Line<float>(p1,p2));
		

		g.setColour(Colours::grey);
		g.drawText(std::to_string(db) + "dB", juce::Rectangle<float>(p1 + Point<float>(2, 2), p1 + Point<float>(50, 10)), Justification::left, false);
	}

	// horizontal grid
	for (int base = 10; base <= fMax; base *= 10)
	{
		g.setColour(Colours::darkgrey);
		for (int dec = 1; dec < 10; dec++)
		{
			float f = base * dec;
			if((f >= fMin) && (f <= fMax)) g.drawLine(Line<float>(Point<float>(f2X(f), dB2Y(dBMin)), Point<float>(f2X(f), dB2Y(dBMax))));
		}

		if (base > fMin)
		{
			g.setColour(Colours::grey);
			auto x = f2X(base);
			std::string label = (base > 1000) ? std::to_string(static_cast<int>(base/1000.f)) + "kHz" : std::to_string(static_cast<int>(base)) + "Hz";
			g.drawText(label, juce::Rectangle<float>(x+2, bounds.getY()+bounds.getHeight()+-14, 50,10), Justification::left, false);

		}
	}

	// draw 0dB line
	g.setColour(Colours::grey);
	g.drawLine(Line<float>(Point<float>(f2X(fMin), dB2Y(0)), Point<float>(f2X(fMax), dB2Y(0))));

	// draw 100Hz, 1kHz, 10kHz
	g.setColour(Colours::grey);
	g.drawLine(Line<float>(Point<float>(f2X(100),   dB2Y(dBMin)), Point<float>(f2X(100),   dB2Y(dBMax))));
	g.drawLine(Line<float>(Point<float>(f2X(1000),  dB2Y(dBMin)), Point<float>(f2X(1000),  dB2Y(dBMax))));
	g.drawLine(Line<float>(Point<float>(f2X(10000), dB2Y(dBMin)), Point<float>(f2X(10000), dB2Y(dBMax))));




	// draw left and right graph 
	for (auto ch : { 0,1 })
	{
		const auto & buffer = (ch == 0) ? ir.getLeftVector() : ir.getRightVector();

		auto colour = (ch == 0) ? juce::Colour(0xff66ffff) : juce::Colour(0xffff6666);

		std::vector<std::complex<float>> fftBuffer;

		for (auto bin : buffer) fftBuffer.push_back(bin);
		fftBuffer.resize(fftSize, 0); // zero pad

		transform->performFFTInPlace(fftBuffer.data());

		g.setColour(colour);

		juce::Point<float> lastPoint(f2X(fMin), dB2Y(mag2db(std::abs(fftBuffer[0]))));

		unsigned int stepSize = (fftBuffer.size() > 4096) ? std::ceil(fftBuffer.size() / 4096.f) : 1;

		for (unsigned int i = 1; i <= 0.5 * fftBuffer.size(); i+= stepSize)
		{
			auto f = ir.getSampleRate() * (static_cast<float>(i) / static_cast<float>(fftBuffer.size()));
			auto dB = mag2db(abs(fftBuffer[i]));

			juce::Point<float> newPoint(f2X(f), dB2Y(dB));

			g.drawLine(Line<float>(lastPoint, newPoint));
			lastPoint = newPoint;
		}
	}
	delete transform;
}

void ImpulseResponseViewComponent::paintImpulseResponse(Graphics & g, juce::Rectangle<int> bounds)
{
	if (ir.getSize() < 2) return;


	auto yMax = getTimeDomainYScale(ir);
	auto yMin = -yMax;

	auto tMin = 0;
	auto tMax = (ir.getSize()-1) / ir.getSampleRate();


	auto t2X = [=](float t)
	{
		auto relative = (t - tMin) / (tMax - tMin);
		return bounds.getX() + 1 + relative * (bounds.getWidth() - 2);
	};

	auto y2Y = [=](float y)
	{
		auto relative = (y - yMin) / (yMax - yMin);
		return bounds.getY() + 1 + (1-relative) * (bounds.getHeight() - 2);
	};

	g.setColour(Colours::darkgrey);
	g.drawLine(Line<float>(Point<float>(t2X(tMin), y2Y(0)), Point<float>(t2X(tMax), y2Y(0))));

	
	for (auto ch : { 0,1 })
	{
		const auto & buffer = (ch == 0) ? ir.getLeftVector() : ir.getRightVector();

		auto colour = (ch == 0) ? juce::Colour(0xff66ffff) : juce::Colour(0xffff6666);

		g.setColour(colour);

		juce::Point<float> lastPoint(t2X(tMin), y2Y(buffer[0]));


		unsigned int stepSize = (buffer.size() > 4096) ? std::ceil(buffer.size() / 4096.f) : 1;

		for (unsigned int i = 1; i < buffer.size(); i+= stepSize)
		{
			auto t = static_cast<float>(i) / ir.getSampleRate();
			auto y = buffer[i];

			juce::Point<float> newPoint(t2X(t), y2Y(y));

			g.drawLine(Line<float>(lastPoint, newPoint));
			lastPoint = newPoint;
		}
	}

}

float ImpulseResponseViewComponent::getTimeDomainYScale(const ImpulseResponse & ir)
{
	float maxAbs = 0;
	for (int i = 0; i < ir.getSize(); i++)
	{
		maxAbs = std::max(std::abs(ir.getLeft()[i]), maxAbs);
		maxAbs = std::max(std::abs(ir.getRight()[i]), maxAbs);
	}

	if (maxAbs == 0) return 1;

	// we take the next power of two
	return std::pow(2.f, std::ceil(std::log2(maxAbs))); 

}

std::pair<float, float> ImpulseResponseViewComponent::getFrequencyDomainDBScale(const ImpulseResponse & ir)
{

	if (ir.getSize() < 2) return { -12,+12 };
	auto irSize = ir.getSize();
	auto fftSize = IRTools::nextPow2(irSize);

	auto transform = AFourierTransformFactory::FourierTransform(std::log2(fftSize));

	auto minDB = +120.f;
	auto maxDB = -120.f;


	for (auto ch : { 0,1 })
	{
		const auto & buffer = (ch == 0) ? ir.getLeftVector() : ir.getRightVector();


		std::vector<std::complex<float>> fftBuffer;

		for (auto bin : buffer) fftBuffer.push_back(bin);
		fftBuffer.resize(fftSize, 0); // zero pad

		transform->performFFTInPlace(fftBuffer.data());

		maxDB = 20.f * std::log10(abs(fftBuffer[0]) + 0.000001f);

		for (unsigned int i = 1; i <= 0.5 * fftBuffer.size(); i++)
		{
			auto dB = 20.f * std::log10(abs(fftBuffer[i]) + 0.000001f);

 			minDB = std::min(dB, minDB);
			maxDB = std::max(dB, maxDB);
		}
	}
	delete transform;

	minDB = std::floor(minDB / 24) * 24;
	maxDB = std::ceil(maxDB / 24) * 24;

	if (maxDB <= minDB)
	{
		maxDB += 12;
		minDB -= 12;
	}
	
	return { minDB, maxDB };
}
