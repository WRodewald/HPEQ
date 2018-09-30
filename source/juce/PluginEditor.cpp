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
    setSize (750, 400);
	constrainer.setMinimumWidth(750);
	constrainer.setMinimumHeight(400);

	this->setConstrainer(&constrainer);
	
	// add sub components
	addAndMakeVisible(fileSelectorComponent);
	addAndMakeVisible(impulseResponseView);

	bool makeResizable = true;
	setResizable(true, makeResizable);
	//if (makeResizable) addAndMakeVisible(resizeComponent);

	fileSelectorComponent.setImpulseResponseFile(processor.getIRFile());

	fileSelectorComponent.onFileChange = [&](juce::File file)
	{
		processor.setIRFile(file);
	};

	processor.setIRUpdateListener(this);

#ifdef WORK_IN_PROGRESS_UI

	// parameters
	setUpToggleButton(controls.invert,		getParameter<AudioParameterBool>("Invert"));
	setUpToggleButton(controls.minPhase,	getParameter<AudioParameterBool>("MinPhase"));
	setUpToggleButton(controls.normalize,	getParameter<AudioParameterBool>("Norm"));
	setUpToggleButton(controls.monoIR,		getParameter<AudioParameterBool>("Mono"));

	setUpComboBox(controls.lowFade,			getParameter<AudioParameterChoice>("LowFade"));
	setUpComboBox(controls.highFade,		getParameter<AudioParameterChoice>("HighFade"));
	setUpComboBox(controls.smooth,			getParameter<AudioParameterChoice>("Smooth"));
	setUpComboBox(controls.engine,			getParameter<AudioParameterChoice>("Engine"));

	setUpComboBox(controls.partitions,		getParameter<AudioParameterInt>("Partitions"));

#endif
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

	unsigned int headerHeight = 38;

#ifdef WORK_IN_PROGRESS_UI
	unsigned int paramViewWidth = 260;
#else
	unsigned int paramViewWidth = 0;
#endif

	unsigned int fileSelectorWidth = 550;

	fileSelectorComponent.setBounds(this->getWidth() *0.5 - 0.5 * fileSelectorWidth, 4, fileSelectorWidth, 30);


	impulseResponseView.setBounds(1, 1 + headerHeight, getWidth() - paramViewWidth - 2, getHeight() - headerHeight - 2);


#ifdef WORK_IN_PROGRESS_UI
	std::vector<Component*> rightSideCtrls = 
	{	&controls.invertButton, 
		&controls.invert, 
		&controls.minPhase,
		&controls.normalize, 
		&controls.monoIR,
		nullptr,
		&controls.lowFade,
		&controls.highFade,
		&controls.smooth,
		&controls.engine,
		&controls.partitions,
	};

	const int ctrlBoxOffsetH = 5;

	const int ctrlOffsetV = 22;
	for (int i = 0; i < rightSideCtrls.size(); i++)
	{
		auto component = rightSideCtrls[i];

		if(component) component->setBounds(ctrlBoxOffsetH + getWidth() - paramViewWidth, headerHeight + i*ctrlOffsetV - 20, paramViewWidth- 2*ctrlBoxOffsetH, 20);
	}

#endif

	resizeComponent.setSize(getWidth(), getHeight());
}

void HpeqAudioProcessorEditor::setUpdateIR(const ImpulseResponse & ir)
{
	this->impulseResponseView.setImpulseResponse(ir);
}

void HpeqAudioProcessorEditor::displayErrorMessage(ErrorMessageType type)
{
	std::string message;
	switch (type)
	{
		case ErrorMessageType::IRCouldNotLoad:
		{
			message = "Impulse Response File could not be loaded.";
		} break;
		case ErrorMessageType::IRToLong:
		{
			message =  getName().toStdString() + " only supports Impulse Responses with a maximum length of " + std::to_string(ConvMaxSize);
		} break;
	}
	AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Erro", message);
}


void HpeqAudioProcessorEditor::setUpToggleButton(ToggleButton & btn, AudioParameterBool * param)
{
	

	jassert(param != nullptr);
	btn.setButtonText(param->name);

	auto response = new ParamListenerT<AudioParameterBool>();

	response->onUpdate = [=, &btn]()
	{
		bool value = param->get();
		btn.setToggleState(value, NotificationType::dontSendNotification);
	};


	btn.onStateChange = [=, &btn]()
	{
		*param = btn.getToggleState();
	};
	
	addAndMakeVisible(btn);
	response->onUpdate();	
}

void HpeqAudioProcessorEditor::setUpComboBox(ComboBoxWLabel & box, AudioParameterChoice * param)
{
	jassert(param != nullptr);
	box.label.setText(param->name, juce::NotificationType::dontSendNotification);

	box.setBoxWidth(150);

	auto response = new ParamListenerT<AudioParameterChoice>();

	response->onUpdate = [=, &box]()
	{
		auto value = param->getCurrentChoiceName();
		for (int i = 0; i < box.box.getNumItems(); i++)
		{
			if (box.box.getItemText(i) == value)
			{
				box.box.setSelectedItemIndex(i);
				return;
			}
		}
	};

	param->addListener(response);
	listeners.push_back(std::shared_ptr<ParamListener>(response));

	int i = 0;
	for (auto choice : param->choices)
	{
		box.box.addItem(choice, 1 + i++);
	}

	box.box.onChange = [param, &box]()
	{
		if(box.box.getSelectedItemIndex() >= 0) *param = box.box.getSelectedItemIndex();
	};

	addAndMakeVisible(box);
	response->onUpdate();
}

void HpeqAudioProcessorEditor::setUpComboBox(ComboBoxWLabel & box, AudioParameterInt * param)
{
	jassert(param != nullptr);
	box.label.setText(param->name, juce::NotificationType::dontSendNotification);

	box.setBoxWidth(150);
	
	std::map<int, int> idToValueMap;
	int id = 1;
	for (int i = param->getRange().getStart(); i <= param->getRange().getEnd(); i++)
	{
		idToValueMap[id] = i;
		box.box.addItem(std::to_string(i), id);
		id++;
	}

	auto response = new ParamListenerT<AudioParameterChoice>();

	response->onUpdate = [=, &box]()
	{
		auto value = param->get();
		for (auto idValuePair : idToValueMap)
		{
			if (idValuePair.second == value)
			{
				box.box.setSelectedId(idValuePair.first);
				return;
			}
		}
	};

	param->addListener(response);
	listeners.emplace_back(response);

	box.box.onChange = [idToValueMap, &box, param]()
	{
		auto id = box.box.getSelectedId();

		if(id > 0) *param = idToValueMap.at(id);
	};

	addAndMakeVisible(box);
	response->onUpdate();
}
