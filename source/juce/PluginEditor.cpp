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
    setSize (400, 300);

	setResizable(false, true);
	

	addAndMakeVisible(fileSelectorComponent);

	addAndMakeVisible(resizeComponent);

	fileSelectorComponent.setImpulseResponseFile(processor.getIRFile());

	fileSelectorComponent.onFileChange = [&](juce::File file)
	{
		processor.setIRFile(file);
	};
}

HpeqAudioProcessorEditor::~HpeqAudioProcessorEditor()
{
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

	fileSelectorComponent.setSize(getWidth(), 30);
	resizeComponent.setSize(getWidth(), getHeight());
}