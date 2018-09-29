/*
  ==============================================================================

    IRFileSelector.cpp
    Created: 20 Jul 2018 10:26:13pm
    Author:  WRodewald

  ==============================================================================
*/

#include "../../JuceLibraryCode/JuceHeader.h"
#include "IRFileSelector.h"

using namespace juce;

//==============================================================================
IRFileSelector::IRFileSelector()
{
	layoutedComponents.push_back(&prevButton);
	layoutedComponents.push_back(nullptr);
	layoutedComponents.push_back(&fileList);
	layoutedComponents.push_back(nullptr);
	layoutedComponents.push_back(&nextButton);
	layoutedComponents.push_back(nullptr);
	layoutedComponents.push_back(&rootButton);
	
	addAndMakeVisible(prevButton);
	addAndMakeVisible(fileList);
	addAndMakeVisible(nextButton);
	addAndMakeVisible(rootButton);

	// callbacks
	rootButton.onClick = [&]() {showDirSelectionDialog(); };
	
	prevButton.onClick = [&]() 
	{
		if (irFiles.size() > 0) setCurrentlySelectedItem((irFiles.size() + currentFileIdx - 1) % irFiles.size());
	};

	nextButton.onClick = [&]()
	{
		if (irFiles.size() > 0) setCurrentlySelectedItem((currentFileIdx + 1) % irFiles.size());
	};

	fileList.onChange = [&]()
	{
		auto curIdx = fileList.getSelectedId() - 1;
		setCurrentlySelectedItem(curIdx);
	};

}

IRFileSelector::~IRFileSelector()
{
}

void IRFileSelector::paint (Graphics& g)
{
}

void IRFileSelector::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

	auto height = getHeight();

	auto spacerSize = 4;

	layout.setItemLayout(0, height, height, height);
	layout.setItemLayout(1, spacerSize, spacerSize, spacerSize);
	layout.setItemLayout(2, 100, 300, -0.6);
	layout.setItemLayout(3, spacerSize, spacerSize, spacerSize);
	layout.setItemLayout(4, height, height, height);
	layout.setItemLayout(5, spacerSize, spacerSize, spacerSize);
	layout.setItemLayout(6, 60, 60, -0.3);
	 

	layout.layOutComponents(layoutedComponents.data(), layoutedComponents.size(), 0, 0, getWidth(), getHeight(), false, true);
}

void IRFileSelector::showDirSelectionDialog()
{
	auto initialPath = data.irRootPath.isDirectory() ? data.irRootPath : File::getCurrentWorkingDirectory();
	FileChooser fileChooser{ "Impulse Response Directory", initialPath };
	bool suc = fileChooser.browseForDirectory();

	if (suc)
	{
		auto dir = fileChooser.getResult();
		setIRRootPath(dir);
	}
}

void IRFileSelector::updateContentsInFileBox()
{
	this->fileList.clear();

	for (int i = 0; i < irFiles.size(); i++)
	{
		fileList.addItem(irFiles[i].getFileName(), i+1);
	}

	if (irFiles.size() > 0)
	{
		setCurrentlySelectedItem(0);
	}
	else
	{
		setCurrentlySelectedItem(-1);
	}
}

void IRFileSelector::setCurrentlySelectedItem(int idx)
{
	currentFileIdx = idx;
	if (idx < 0)
	{
		fileList.setSelectedId(0, NotificationType::dontSendNotification); 
		callCallback();
	}
	else
	{
		fileList.setSelectedId(idx+1, NotificationType::dontSendNotification);
		callCallback();
		data.irFile = irFiles[currentFileIdx];
	}
}

void IRFileSelector::callCallback()
{
	if (onFileChange)
	{
		if (currentFileIdx >= 0)
		{			
			jassert(currentFileIdx < irFiles.size());
			onFileChange(irFiles[currentFileIdx]);

		}
		else
		{
			// call with emtpy / invalid file to signal that no file is loaded
			onFileChange(File());
		}
	}
}


void IRFileSelector::setImpulseResponseFile(juce::File path)
{
	if (path.isDirectory())
	{
		setIRRootPath(path);
	}
	else
	{
		// disable callback
		auto callbackStore = this->onFileChange;
		onFileChange = nullptr;

		// set parent path
		auto dir = path.getParentDirectory();
		setIRRootPath(dir);

		// find and set current item
		for (int i = 0; i < irFiles.size(); i++)
		{
			if (irFiles[i].getFileName() == path.getFileName())
			{
				setCurrentlySelectedItem(i);
				break;
			}
		}

		// reinstall callback
		this->onFileChange = callbackStore;
	}
}

void IRFileSelector::setIRRootPath(juce::File path)
{
	if (data.irRootPath == path) return;
	data.irRootPath = path;

	auto searchFlags = juce::File::TypesOfFileToFind::findFiles | juce::File::TypesOfFileToFind::ignoreHiddenFiles;
	irFiles = path.findChildFiles(searchFlags, false, "*.wav");

	updateContentsInFileBox();
	
}
