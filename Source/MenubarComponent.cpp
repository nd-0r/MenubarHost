#include "MenubarComponent.h"

MenubarComponent::MenubarComponent(juce::JUCEApplication& app,
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
  // Nothing
}

void MenubarComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown())
    {
      juce::PopupMenu pm;
      addPluginsToMenu(pm);
      pm.addItem("Preferences", [this] () {
        settings_window_ = new SettingsWindow(adm_);
      });
      pm.addItem("Quit", [this] () {
        app_.quit();
      });
      showDropdownMenu(pm);
    }
    else if (event.mods.isRightButtonDown())
    {
      rack_editor_window_ = new RackEditorWindow(pm_);
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

void MenubarComponent::addPluginsToMenu(juce::PopupMenu& popup_menu)
{
  auto& active_plugins = pm_.getActivePlugins();
  if (active_plugins.isEmpty())
  {
    juce::PopupMenu::Item item;
    item.itemID = -1;
    item.text = "No plugins active";
    item.colour = juce::Colours::dimgrey; // TODO fix
    popup_menu.addItem(item);
  }
  for (auto& plugin : active_plugins)
  {
    popup_menu.addItem(plugin->description.name, [] () {});
  }
  popup_menu.addSeparator();
}
