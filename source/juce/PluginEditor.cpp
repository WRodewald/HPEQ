/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HpeqAudioProcessorEditor::HpeqAudioProcessorEditor (HpeqAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);


		
	// add sub components
	addAndMakeVisible(fileSelectorComponent);	
	addAndMakeVisible(paramEditorComponent);
	addAndMakeVisible(impulseResponseView);

	bool makeResizable = true;
	setResizable(true, makeResizable);
	if (makeResizable) addAndMakeVisible(resizeComponent);

	fileSelectorComponent.setImpulseResponseFile(processor.getIRFile());

	fileSelectorComponent.onFileChange = [&](juce::File file)
	{
		processor.setIRFile(file);
	};

	processor.setIRUpdateListener(this);

}

HpeqAudioProcessorEditor::~HpeqAudioProcessorEditor()
{
	processor.setIRUpdateListener(nullptr);
}

//==============================================================================
void HpeqAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

}

void HpeqAudioProcessorEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..

	fileSelectorComponent.setBounds(200,0,getWidth()- 200, 30);

	impulseResponseView.setBounds(200, 30, getWidth()- 200, getHeight()-30);

	paramEditorComponent.setBounds(0, 0, 200, getHeight());

	resizeComponent.setSize(getWidth(), getHeight());
}

void HpeqAudioProcessorEditor::setUpdateIR(const ImpulseResponse & ir)
{
	this->impulseResponseView.setImpulseResponse(ir);
}
