#include "MenubarComponent.h"

MenubarComponent::MenubarComponent(MenubarHostApplication& app,
                                   ProcessingManager& pm,
                                   juce::AudioDeviceManager& adm) :
    app_(app), pm_(pm), adm_(adm)
{
  const auto image = juce::ImageFileFormat::loadFrom(
      BinaryData::GenericSpeaker_png, BinaryData::GenericSpeaker_pngSize);
  setIconImage(image, image);
  setIconTooltip(app_.getApplicationName());
  juce::MenuBarModel::setMacMainMenu(this);
  // initialize processing manager
  juce::String err_text = pm_.initialize();
  if (!err_text.isEmpty()) {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "Error",
                                           "Cannot find audio devices",
                                           "Quit",
                                           nullptr,
                                           this);
  }
  setIconTooltip(juce::JUCEApplication::getInstance()->getApplicationName());
}

MenubarComponent::~MenubarComponent()
{
  settings_window_.deleteAndZero();
  rack_editor_window_.deleteAndZero();
  setMacMainMenu(nullptr);
}

juce::StringArray MenubarComponent::getMenuBarNames()
{
  return juce::StringArray();
}

juce::PopupMenu MenubarComponent::getMenuForIndex(int topLevelMenuIndex,
                                                  const juce::String& menuName)
{
  return juce::PopupMenu();
}

void MenubarComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
  std::cout << "Menu ID: " << menuItemID << std::endl;
  switch (menuItemID)
  {
    case 1: // Quit
      app_.quit();
      break;
    case 2: // Preferences
      if (!settings_window_.getComponent())
        settings_window_ = new SettingsWindow(adm_);
      break;
    default: // Plugin choice
      if (menuItemID < 67) // Active plugin choice
      {
        juce::String selected_plugin = pm_.getActivePlugins()[menuItemID - 3]->description.name;
        std::cout << selected_plugin << std::endl;
      }
      else // Available plugin choice
      {
        pm_.addPlugin(KnownPluginList::getIndexChosenByMenu(
            pm_.getAvailablePlugins().getTypes(), menuItemID
        ));
      }
  }
}

void MenubarComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown())
    {
      juce::PopupMenu plugin_stack_popup;
 
      addActivePluginsToMenu(plugin_stack_popup);
 
      juce::PopupMenu available_plugins_submenu;
      KnownPluginList::addToMenu(available_plugins_submenu,
                                 pm_.getAvailablePlugins().getTypes(),
                                 KnownPluginList::SortMethod::sortByCategory);
      plugin_stack_popup.addSubMenu("Add plugin", available_plugins_submenu);

      plugin_stack_popup.addSeparator();

      plugin_stack_popup.addItem(2, "Preferences");
      plugin_stack_popup.addItem(1, "Quit");
      showDropdownMenu(plugin_stack_popup);
    }
    else if (event.mods.isRightButtonDown())
    {
      rack_editor_window_ = new RackEditorWindow(app_.getApplicationProperties().getUserSettings(), pm_);
    }
}

void MenubarComponent::mouseUp(const juce::MouseEvent& event)
{
}

void MenubarComponent::modalStateFinished(int returnValue)
{
  app_.quit();
}

// private =====================================================================
void MenubarComponent::handleOpenPlugin(juce::PluginDescription& pd)
{
  // TODO
}

void MenubarComponent::addActivePluginsToMenu(juce::PopupMenu& popup_menu)
{
  auto& active_plugins = pm_.getActivePlugins();
  // Offset to make room for 'Preferences' and 'Quit'
  // 0 reserved to mean 'Nothing selected'
  int active_plugin_idx = 3;
  for (auto& plugin : active_plugins)
  {
    popup_menu.addItem(active_plugin_idx++, plugin->description.name);
  }
}
