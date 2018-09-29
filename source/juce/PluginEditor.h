/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

#include "IRFileSelector.h"
#include "ImpulseResponseViewComponent.h"

//==============================================================================
/*
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
class ParamListenerT: public ParamListener, public T::Listener
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




//==============================================================================
/**
*/
class HpeqAudioProcessorEditor  : public AudioProcessorEditor,  public ImpulseResponseUpdateListener
{
public:

	enum class ErrorMessageType
	{
		IRToLong,
		IRCouldNotLoad
	};
public:
    HpeqAudioProcessorEditor (HpeqAudioProcessor&);
    ~HpeqAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

	virtual void setUpdateIR(const ImpulseResponse & ir) override;

	void displayErrorMessage(ErrorMessageType type);

private:

	template<typename T>
	T * getParameter(std::string name);


	void setUpToggleButton(ToggleButton & btn, AudioParameterBool * param);
	void setUpComboBox(ComboBoxWLabel &box, AudioParameterChoice * param);
	void setUpComboBox(ComboBoxWLabel &box, AudioParameterInt * param);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    HpeqAudioProcessor& processor;

	IRFileSelectorComponent fileSelectorComponent;
	
	ImpulseResponseViewComponent impulseResponseView;
	
	juce::ResizableBorderComponent resizeComponent{ this, nullptr };
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HpeqAudioProcessorEditor)


	struct
	{
		ToggleButton invertButton;
		ToggleButton invert;
		ToggleButton minPhase;	
		ToggleButton normalize;
		ToggleButton monoIR;	

		ComboBoxWLabel lowFade;		
		ComboBoxWLabel highFade;
		ComboBoxWLabel smooth;
		ComboBoxWLabel engine;
		ComboBoxWLabel partitions;
		
	} controls; 


	std::vector<std::shared_ptr<ParamListener>> listeners;


	juce::ComponentBoundsConstrainer constrainer;
};


template<typename T>
inline T * HpeqAudioProcessorEditor::getParameter(std::string name)
{
	auto & parameters = processor.getParameters();

	for (auto p : parameters)
	{
		auto parameter = dynamic_cast<T*>(p);
		if (parameter && (parameter->paramID == String(name))) return parameter;
	}
	return nullptr;
}
