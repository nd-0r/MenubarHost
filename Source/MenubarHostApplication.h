/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginScannerSubprocess.hpp"
#include "CustomPluginScanner.hpp"
#include "ProcessingManager.h"
#include "MenubarComponent.h"

class MenubarComponent;

//==============================================================================
class MenubarHostApplication  : public juce::JUCEApplication
{
public:
//==============================================================================
  MenubarHostApplication();

  const juce::String getApplicationName() override;

  const juce::String getApplicationVersion() override;
  
  ApplicationProperties& getApplicationProperties();

  bool moreThanOneInstanceAllowed() override;

//==============================================================================
  void initialise (const juce::String& commandLine) override;

  void shutdown() override;

//==============================================================================
  void systemRequestedQuit() override;

  void anotherInstanceStarted (const juce::String& commandLine) override;

private:
  std::unique_ptr<PluginScannerSubprocess> storedScannerSubprocess;
  std::unique_ptr<MenubarComponent> menubarComponent;
  AudioDeviceManager deviceManager;
  ApplicationProperties appProperties;
  ProcessingManager processingManager =
      ProcessingManager{appProperties, deviceManager};
};
