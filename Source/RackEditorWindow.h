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
  RackEditorWindow(PropertiesFile* user_settings, ProcessingManager& pm);
  ~RackEditorWindow() override;

  void closeButtonPressed() override;
private:
  PropertiesFile* user_settings_;
  ProcessingManager& pm_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackEditorWindow)
};
