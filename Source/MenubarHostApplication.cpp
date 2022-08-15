/*
  ==============================================================================

    MenubarHostApplication.cpp
    Created: 7 Jul 2022 9:19:05pm
    Author:  Andrew Orals

  ==============================================================================
*/

#include "MenubarHostApplication.h"

MenubarHostApplication::MenubarHostApplication() { /* Nothing */ }

const juce::String MenubarHostApplication::getApplicationName()
{
  return ProjectInfo::projectName;
}

const juce::String MenubarHostApplication::getApplicationVersion()
{
  return ProjectInfo::versionString;
}

ApplicationProperties& MenubarHostApplication::getApplicationProperties()
{
  return *appProperties;
}

bool MenubarHostApplication::moreThanOneInstanceAllowed()
{
  return true;
}

void MenubarHostApplication::initialise (const juce::String& commandLine)
{
  // From JUCE AudioPluginHost
  // https://github.com/juce-framework/JUCE/blob/master/extras/AudioPluginHost/Source/UI/MainHostWindow.cpp
  auto scannerSubprocess = std::make_unique<PluginScannerSubprocess>();
  
  if (scannerSubprocess->initialiseFromCommandLine(commandLine, processUID))
  {
    storedScannerSubprocess = std::move(scannerSubprocess);
    return;
  }
  // End From JUCE AudioPluginHost
  
  PropertiesFile::Options propOptions;
  propOptions.applicationName     = getApplicationName();
  propOptions.filenameSuffix      = "settings";
  propOptions.osxLibrarySubFolder = "Preferences";
  appProperties.reset(new ApplicationProperties());
  appProperties->setStorageParameters(propOptions);
  
  deviceManager = std::make_unique<AudioDeviceManager>();
  processingManager = std::make_unique<ProcessingManager>(*appProperties,
                                                          *deviceManager);
  menubarComponent = std::make_unique<MenubarComponent>(*this,
                                                        *processingManager,
                                                        *deviceManager);
}

void MenubarHostApplication::shutdown()
{
  menubarComponent = nullptr;
}

void MenubarHostApplication::systemRequestedQuit()
{
  quit();
}

void MenubarHostApplication::anotherInstanceStarted (
    const juce::String& commandLine)
{
  // When another instance of the app is launched while this one is running,
  // this method is invoked, and the commandLine parameter tells you what
  // the other instance's command-line arguments were.
}
