/*
  ==============================================================================

    SettingsWindow.h
    Created: 30 Jun 2022 11:26:50pm
    Author:  Andrew Orals

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

class SettingsWindow : public juce::DocumentWindow
{
public:
  SettingsWindow(AudioDeviceManager& device_manager) :
      DocumentWindow("Settings",
                     Colours::grey,
                     DocumentWindow::TitleBarButtons::closeButton,
                     false),
      device_selector_(device_manager,
                       0,     // minimum input channels
                       2,   // maximum input channels
                       1,     // minimum output channels
                       2,   // maximum output channels
                       true,  // ability to select midi inputs
                       true,  // ability to select midi output device
                       false, // treat channels as stereo pairs
                       false) // hide advanced input options
  // from https://docs.juce.com/master/tutorial_audio_device_manager.html
  {
    setContentOwned(&device_selector_, false);
    cbs_.setSizeLimits(600, 400, 600, 600);
    setResizable(true, true);
    setConstrainer(&cbs_);
    addToDesktop();
    setVisible(true);
  }

  void closeButtonPressed() override
  {
    delete this;
  }
private:
  ComponentBoundsConstrainer cbs_;
  AudioDeviceSelectorComponent device_selector_;
};
