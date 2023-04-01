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
                                   ProcessingManager& pm,
                                   const juce::File& failed_plugins_file) :
    juce::DocumentWindow("Edit Plugin Configuration",
                         juce::Colours::grey,
                         DocumentWindow::TitleBarButtons::closeButton,
                         false),
    user_settings_(user_settings),
    pm_(pm),
    failed_plugins_file_(failed_plugins_file)
{
  setContentOwned(
      new PluginListComponent(pm_.getPluginFormatManager(),
                              pm_.getAvailablePlugins(),
                              failed_plugins_file_,
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
