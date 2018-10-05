#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

class GenericTimer : public Timer
{
public:
	std::function<void(void)> callback{ nullptr };
private:
	// Inherited via Timer
	virtual void timerCallback() override
	{
		if (callback) callback();
	}
};

//==============================================================================
/**

	A small icon component that shows a busy state animation. Has a auto callback feature that will poll the sate
	automatically.
*/
class BusyIcon    : public Component
{
public:
	enum class State
	{
		Idle, Busy
	};

public:
    BusyIcon();
    ~BusyIcon();

    void paint (Graphics&) override;
    void resized() override;

	/**
		If called, the state poll timer will be enabled and will call the callback set with #setStateCallback.
		Can be disable with #disableAutoUpdater
		@params intervalMs the interval in milliseconds between each state pool.
	*/
	void enableAutoUpdater(float intervalMs);

	/**
		If called, the state poll timer will be disabled. Can be enabled with #enableAutoUpdater.
	*/
	void disableAutoUpdater();

	/**
		Sets the callback function that is called by the state poll timer.
		@params callback the callback function. If returns true, the icon will display the busy animation.
	*/
	void setStateCallback(std::function<State(void)> callback);

	/**
		Sets the state manaully and start animation. The state will be overwritten by the state poll timer if used in conjunction. 
	*/
	void setState(State state);

	/**
		Sets the apearence of the busy icon.
		@param colour the colour of the circles
		@param numCircles the nuber of displayed circles
		@param circleRadiusRelative the size of the biggest circle relative to the min(width,height) of the component
		@param circleFalloff the ratio between each circle and the next circle (exponential radius falloff)
	*/
	void setAppearance(juce::Colour colour, unsigned int numCircles = 8, float circleRadiusRelative = 0.2, float circleFalloff = 0.95);


private:

	static float getAngle();

private:

	GenericTimer repaintTimer;
	GenericTimer statePollTimer;

	std::function<State(void)> callback{ nullptr };
	State state{ State::Idle };

	juce::Colour colour;
	unsigned int numCircles{ 8 };
	float circleRadiusRelative{ 0.1f };
	float circleFalloff{ 0.9f };



private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BusyIcon)

};
