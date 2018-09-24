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
/**
*/
class HpeqAudioProcessorEditor  : public AudioProcessorEditor,  public ImpulseResponseUpdateListener
{
public:
    HpeqAudioProcessorEditor (HpeqAudioProcessor&);
    ~HpeqAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

	virtual void setUpdateIR(const ImpulseResponse & ir) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    HpeqAudioProcessor& processor;

	IRFileSelector fileSelectorComponent;
	
	ImpulseResponseViewComponent impulseResponseView;

	// displays all the parameters
	juce::GenericAudioProcessorEditor paramEditorComponent{ &processor };

	juce::ResizableBorderComponent resizeComponent{ this, nullptr };


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HpeqAudioProcessorEditor)

};
