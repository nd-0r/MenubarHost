/*
  ==============================================================================

    RackEditor.cpp
    Created: 30 Jun 2022 10:25:38pm
    Author:  Andrew Orals

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginListWindow.h"

//==============================================================================
PluginListWindow::PluginListWindow(PropertiesFile* user_settings,
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
  setUsingNativeTitleBar(true);
  setResizable(true, true);
  addToDesktop();
  setVisible(true);
}

PluginListWindow::~PluginListWindow()
{
}

void PluginListWindow::closeButtonPressed()
{
  delete this;
}
