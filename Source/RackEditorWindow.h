/*
  ==============================================================================

    RackEditor.h
    Created: 30 Jun 2022 10:25:38pm
    Author:  Andrew Orals

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ProcessingManager.h"

//==============================================================================
/*
*/
class RackEditorWindow  : public juce::DocumentWindow
{
public:
  RackEditorWindow(ProcessingManager& pm);
  ~RackEditorWindow() override;

  void closeButtonPressed() override;
private:
  ProcessingManager& pm_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackEditorWindow)
};
