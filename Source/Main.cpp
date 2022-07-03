/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ProcessingManager.h"
#include "MenubarComponent.h"

//==============================================================================
class MenubarHostApplication  : public juce::JUCEApplication
{
public:
//==============================================================================
  MenubarHostApplication() {}

  const juce::String getApplicationName() override
  {
    return ProjectInfo::projectName;
  }

  const juce::String getApplicationVersion() override
  {
    return ProjectInfo::versionString;
  }

  bool moreThanOneInstanceAllowed() override
  {
    return true;
  }

//==============================================================================
  void initialise (const juce::String& commandLine) override
  {
    menubarComponent = std::unique_ptr<MenubarComponent>(
        new MenubarComponent(*this, processingManager, deviceManager)
    );
  }

  void shutdown() override
  {
    menubarComponent = nullptr;
  }

//==============================================================================
  void systemRequestedQuit() override
  {
    quit();
  }

  void anotherInstanceStarted (const juce::String& commandLine) override
  {
    // When another instance of the app is launched while this one is running,
    // this method is invoked, and the commandLine parameter tells you what
    // the other instance's command-line arguments were.
  }

private:
  std::unique_ptr<MenubarComponent> menubarComponent;
  AudioDeviceManager deviceManager;
  ProcessingManager processingManager = ProcessingManager{deviceManager};
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (MenubarHostApplication)
