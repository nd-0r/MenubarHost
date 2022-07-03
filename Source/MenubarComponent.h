#pragma once

#include <JuceHeader.h>
#include "ProcessingManager.h"
#include "RackEditorWindow.h"
#include "SettingsWindow.hpp"

class MenubarComponent  : public juce::SystemTrayIconComponent,
                          public juce::MenuBarModel,
                          public juce::ModalComponentManager::Callback
{
public:
  MenubarComponent(juce::JUCEApplication& app, ProcessingManager& pm, juce::AudioDeviceManager& adm);
  ~MenubarComponent();
  
  //====start MenuBarModel Implementations====
  juce::StringArray getMenuBarNames() override;
  juce::PopupMenu getMenuForIndex(int topLevelMenuIndex,
                                  const juce::String& menuName) override;
  void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
  //====end MenuBarModel Implementations====
  
  void mouseDown(const juce::MouseEvent& event) override;
  void mouseUp(const juce::MouseEvent& event) override;

  void modalStateFinished(int returnValue) override;
private:
  void addPluginsToMenu(juce::PopupMenu& popup_menu);
  juce::JUCEApplication& app_;
  ProcessingManager& pm_;
  juce::AudioDeviceManager& adm_;
  juce::Component::SafePointer<RackEditorWindow> rack_editor_window_;
  juce::Component::SafePointer<SettingsWindow> settings_window_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenubarComponent)
};
