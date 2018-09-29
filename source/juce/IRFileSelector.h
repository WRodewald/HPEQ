/*
  ==============================================================================

    IRFileSelector.h
    Created: 20 Jul 2018 10:26:13pm
    Author:  WRodewald

  ==============================================================================
*/

#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

/** TODO
	- make generic file selector, class doesn't need to know about what type of file is slected	
*/

//==============================================================================

/**
	A component that provides UI components to navigate between multiple .wav impulse responses in a selectable folder
*/
class IRFileSelectorComponent    : public Component
{

public:
    IRFileSelectorComponent();
    ~IRFileSelectorComponent();

    void paint (Graphics&) override;
    void resized() override;

	/**
		A callback function that can be assigned. Is called once the selected impulse response file was updated
	*/
	std::function<void(juce::File)> onFileChange; 


	/**
		sets the impulse response file and if necessary updates the contianing directory
		@param path the path to a .wav file
	*/
	void setImpulseResponseFile(juce::File path);
	
private:

	void setIRRootPath(juce::File path);

	void showDirSelectionDialog();

	void updateContentsInFileBox();
	void setCurrentlySelectedItem(int idx);

	void callCallback();

	
private:
	
	juce::ArrowButton prevButton{ "PrevButton", 0.5, juce::Colour(0xffffffff) };
	juce::ArrowButton nextButton{ "NextButton", 0.0, juce::Colour(0xffffffff) };
	juce::ComboBox    fileList{ "FileList" };
	juce::TextButton  rootButton{ "Select Folder", "select the path containing impuls responses" };

	juce::StretchableLayoutManager layout;
	std::vector<juce::Component*>  layoutedComponents;


	juce::Array<juce::File> irFiles;

	// currently selected file from file List, or non-selected if negative
	int currentFileIdx{ -1 };


	// Contains data that the component stores in plugin state for neatness
	struct Data
	{
		juce::File irRootPath;
		juce::File irFile;

	} data;



private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRFileSelectorComponent)
};
