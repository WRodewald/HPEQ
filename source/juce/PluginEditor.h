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
#include "BusyIcon.h"
#include "JuceUtility.h"

// the parameter handlin in the view seems to be broken currenlty
// works on Windows but not sufficiently tested on MacOS, thus, removed for now
//#define WORK_IN_PROGRESS_UI 



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

	BusyIcon busyIcon;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HpeqAudioProcessorEditor)


#ifdef WORK_IN_PROGRESS_UI



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

#endif


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
