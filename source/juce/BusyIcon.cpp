
#define _USE_MATH_DEFINES
#include <cmath>
#include <ctime>

#include "BusyIcon.h"

//==============================================================================
BusyIcon::BusyIcon()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.


}

BusyIcon::~BusyIcon()
{
}

void BusyIcon::paint (Graphics& g)
{
	if (state != State::Busy) return;

	float angleOffset = 2 * M_PI / static_cast<float>(numCircles);

	auto angle = getAngle();

	auto center = getBounds().getCentre().toFloat() - getBounds().getTopLeft().toFloat();
	auto radius = 0.45 * (1.f - circleRadiusRelative) * std::min(this->getWidth(), this->getHeight());

	float circleRadius = circleRadiusRelative * std::min(this->getWidth(), this->getHeight());

	g.setColour(colour);

	for (int i = 0; i < numCircles; i++)
	{
		float circleAngle = angle + i * angleOffset;
		Point<float> pos(std::cos(circleAngle), std::sin(circleAngle));
		pos = center + pos * radius;

		g.fillEllipse(pos.x - 0.5*circleRadius, pos.y - 0.5 *circleRadius, circleRadius, circleRadius);

		circleRadius *= circleFalloff;
	}
}

void BusyIcon::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

void BusyIcon::enableAutoUpdater(float intervalMs)
{
	statePollTimer.callback = [this]() 
	{
		if (callback) setState(callback());

	};
	statePollTimer.startTimer(intervalMs);
}

void BusyIcon::disableAutoUpdater()
{
	statePollTimer.stopTimer();
}

void BusyIcon::setStateCallback(std::function<State(void)> callback)
{
	this->callback = callback;
}

void BusyIcon::setState(State state)
{
	this->state = state;
	if (state == State::Busy)
	{
		this->repaintTimer.startTimerHz(60);
		repaintTimer.callback = [this]() {repaint(); };
	}
	else
	{
		repaintTimer.stopTimer();
	}
	repaint();
}

void BusyIcon::setAppearance(juce::Colour colour, unsigned int numCircles, float circleRadiusRelative, float circleFalloff)
{
	this->colour			   = colour;
	this->numCircles		   = std::max(1U, numCircles);
	this->circleRadiusRelative = std::max(0.f, std::min(1.f, circleRadiusRelative));
	this->circleFalloff		   = std::max(0.f, std::min(1.f, circleFalloff));
}

float BusyIcon::getAngle()
{
	
	double  nowSec = std::clock() / static_cast<double>(CLOCKS_PER_SEC);
	double  time   = nowSec - static_cast<uint64_t>(nowSec);

	
	return 2 * M_PI * time;
}
