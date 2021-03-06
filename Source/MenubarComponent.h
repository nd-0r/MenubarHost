#pragma once

#include <JuceHeader.h>
#include "MenubarHostApplication.h"
#include "ProcessingManager.h"
#include "RackEditorWindow.h"
#include "SettingsWindow.hpp"

class MenubarComponent  : public juce::SystemTrayIconComponent,
                          public juce::MenuBarModel,
                          public juce::ModalComponentManager::Callback
{
public:
  MenubarComponent(MenubarHostApplication& app,
                   ProcessingManager& pm,
                   juce::AudioDeviceManager& adm);
  ~MenubarComponent();
  
  //====start MenuBarModel Implementations====
  juce::StringArray getMenuBarNames() override;
  juce::PopupMenu getMenuForIndex(int topLevelMenuIndex,
                                  const juce::String& menuName) override;
  /**
   *  Menu indexing scheme:
   *  Active plugins: 3..n
   *  Preferences: 2
   *  Quit: 1
   */
  void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
  //====end MenuBarModel Implementations====
  
  void mouseDown(const juce::MouseEvent& event) override;
  void mouseUp(const juce::MouseEvent& event) override;

  void modalStateFinished(int returnValue) override;
private:
  class PluginCallback : public juce::PopupMenu::CustomCallback
  {
    std::function<void(juce::PluginDescription&)> handler_;
    juce::PluginDescription& plugin_;
  public:
    PluginCallback(std::function<void(juce::PluginDescription&)> handler,
                   juce::PluginDescription& plugin) :
        handler_(handler), plugin_(plugin) { /* Nothing */ }

    bool menuItemTriggered() override
    {
      handler_(plugin_);
      return true; // TODO
    }
  };
 
  void handleOpenPlugin(juce::PluginDescription& pd);
  void addActivePluginsToMenu(juce::PopupMenu& popup_menu);
  void addAvailablePluginsToMenu(juce::PopupMenu& popup_menu);
 
  MenubarHostApplication& app_;
  ProcessingManager& pm_;
  juce::AudioDeviceManager& adm_;
  juce::Component::SafePointer<RackEditorWindow> rack_editor_window_;
  juce::Component::SafePointer<SettingsWindow> settings_window_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenubarComponent)
};
