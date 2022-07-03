/*
  ==============================================================================

    HostWindow.h
    Created: 14 Jan 2022 12:55:10am
    Author:  Andrew Orals

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class HostWindow  : public juce::Component
{
public:
  HostWindow();
  ~HostWindow() override;

  void paint (juce::Graphics&) override;
  void resized() override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HostWindow)
};
