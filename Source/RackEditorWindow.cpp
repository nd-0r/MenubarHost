/*
  ==============================================================================

    RackEditor.cpp
    Created: 30 Jun 2022 10:25:38pm
    Author:  Andrew Orals

  ==============================================================================
*/

#include <JuceHeader.h>
#include "RackEditorWindow.h"

//==============================================================================
RackEditorWindow::RackEditorWindow(PropertiesFile* user_settings,
                                   ProcessingManager& pm) :
    juce::DocumentWindow("Edit Plugin Configuration",
                         juce::Colours::grey,
                         DocumentWindow::TitleBarButtons::closeButton,
                         false),
    user_settings_(user_settings),
    pm_(pm)
{
  const File f = File("/Users/andreworals/Downloads/dmpf");
  setContentOwned(
      new PluginListComponent(pm_.getPluginFormatManager(),
                              pm_.getAvailablePlugins(),
                              f,
                              user_settings_,
                              true),
      true
  );
  setResizable(true, true);
  addToDesktop();
  setVisible(true);
}

RackEditorWindow::~RackEditorWindow()
{
}

void RackEditorWindow::closeButtonPressed()
{
  delete this;
}
