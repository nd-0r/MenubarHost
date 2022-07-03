/*
  ==============================================================================

    HostWindow.cpp
    Created: 14 Jan 2022 12:55:10am
    Author:  Andrew Orals

  ==============================================================================
*/

#include <JuceHeader.h>
#include "HostWindow.h"

//==============================================================================
HostWindow::HostWindow()
{
}

HostWindow::~HostWindow()
{
}

void HostWindow::paint (juce::Graphics& g)
{
  g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

  g.setColour (juce::Colours::grey);
  g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

  g.setColour (juce::Colours::white);
  g.setFont (14.0f);
  g.drawText ("HostWindow", getLocalBounds(),
              juce::Justification::centred, true);   // draw some placeholder text
}

void HostWindow::resized()
{
}
