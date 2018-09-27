/*
  ==============================================================================

    ImpulseResponseViewComponent.h
    Created: 24 Sep 2018 1:19:17pm
    Author:  WRodewald

  ==============================================================================
*/

#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"
#include "../hpeq/ImpulseResponse.h"


//==============================================================================
/*
*/
class ImpulseResponseViewComponent    : public Component
{
public:
    ImpulseResponseViewComponent();
    ~ImpulseResponseViewComponent();

	void setImpulseResponse(ImpulseResponse ir);

    void paint (Graphics&) override;
    void resized() override;

private:

	void painSpectrum(Graphics &g, juce::Rectangle<int> bounds);
	void paintImpulseResponse(Graphics &g, juce::Rectangle<int> bounds);


	static float getTimeDomainYScale(const ImpulseResponse & ir);


	static std::pair<float, float> getFrequencyDomainDBScale(const ImpulseResponse & ir);

private:
	
	ImpulseResponse ir;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpulseResponseViewComponent)
};
