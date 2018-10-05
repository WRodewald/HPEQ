#pragma once

#include "JuceHeader.h"


/**
	Implements a combobox component with adjecent label.
*/
class ComboBoxWLabel : public juce::Component
{
public:
	ComboBoxWLabel()
	{
		addAndMakeVisible(box);
		addAndMakeVisible(label);
	}

	void setBoxWidth(int width)
	{
		this->boxWidth = width;
		resized();
	}

public:

	Label label;
	ComboBox box;

protected:


	void resized() override
	{
		box.setBounds(0, 0, boxWidth, getHeight());
		label.setBounds(boxWidth + 2, 0, getWidth() - boxWidth - 2, getHeight());
	}


private:
	unsigned int boxWidth{ 100 };

};



//@cond  
class ParamListener
{
public:
	virtual ~ParamListener() = default;
};
//@endcon




/**
Implements a parameter listener that forwards to a generic std::function on change
*/
template<typename T>
class ParamListenerT : public ParamListener, public T::Listener
{
public:
	virtual ~ParamListenerT() = default;

	std::function<void(void)> onUpdate;

protected:
	virtual void parameterValueChanged(int parameterIndex, float newValue) override
	{
		if (onUpdate) onUpdate();
	}

	virtual void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override
	{
		if (onUpdate) onUpdate();
	}
};
