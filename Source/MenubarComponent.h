#pragma once

#include <JuceHeader.h>
#include "MenubarHostApplication.h"
#include "ProcessingManager.h"
#include "PluginListWindow.h"
#include "RackEditorWindow.h"
#include "SettingsWindow.hpp"
#include "PluginWindow.h"

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
 
  PluginWindow* getOrCreatePluginWindow(juce::AudioProcessorGraph::Node* p,
                                        PluginWindow::Type t);
  bool closeOpenPluginWindows();
  void handleOpenPlugin(juce::AudioProcessorGraph::Node* p);
  void handleRemovePlugin(juce::AudioProcessorGraph::Node* p);
  void addActivePluginsToMenu(juce::PopupMenu& popup_menu);
  void addAvailablePluginsToMenu(juce::PopupMenu& popup_menu);
 
  MenubarHostApplication& app_;
  ProcessingManager& pm_;
  juce::AudioDeviceManager& adm_;
  juce::Component::SafePointer<PluginListWindow> plugin_list_window_;
  juce::Component::SafePointer<SettingsWindow> settings_window_;
  juce::Component::SafePointer<RackEditorWindow> plugin_rack_editor_;
  juce::OwnedArray<PluginWindow> active_plugin_windows_;
  juce::TemporaryFile failed_plugins_file_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenubarComponent)
};
