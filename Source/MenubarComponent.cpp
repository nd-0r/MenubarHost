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
  setIconTooltip(juce::JUCEApplication::getInstance()->getApplicationName());
}

MenubarComponent::~MenubarComponent()
{
  settings_window_.deleteAndZero();
  plugin_list_window_.deleteAndZero();
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
  switch (menuItemID)
  {
    case 1: // Quit
      app_.quit();
      break;
    case 2: // Preferences
      if (!settings_window_.getComponent())
        settings_window_ = new SettingsWindow(adm_,
                                              app_.getApplicationProperties());
      break;
    case 3:
      if (!plugin_rack_editor_.getComponent())
        plugin_rack_editor_ = new RackEditorWindow(pm_.getActivePlugins());
    default: // Available plugin choice
      pm_.addPlugin(KnownPluginList::getIndexChosenByMenu(
          pm_.getAvailablePlugins().getTypes(), menuItemID
      ));
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
      plugin_stack_popup.addItem(3, "Edit Configuration");
      plugin_stack_popup.addItem(2, "Preferences");
      plugin_stack_popup.addItem(1, "Quit");

      showDropdownMenu(plugin_stack_popup);
    }
    else if (event.mods.isRightButtonDown())
    {
      plugin_list_window_ = new PluginListWindow(
          app_.getApplicationProperties().getUserSettings(), pm_
      );
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

// From JUCE AudioPluginHost
// https://github.com/juce-framework/JUCE/blob/master/extras/AudioPluginHost/Source/UI/MainHostWindow.cpp
PluginWindow* MenubarComponent::getOrCreatePluginWindow(
    juce::AudioProcessorGraph::Node* p,
    PluginWindow::Type t)
{
    jassert (p != nullptr);

    for (auto* w : active_plugin_windows_)
        if (w->node.get() == p && w->type == t)
            return w;

    if (auto* processor = p->getProcessor())
    {
        if (auto* plugin = dynamic_cast<AudioPluginInstance*>(processor))
        {
            auto description = plugin->getPluginDescription();

            return active_plugin_windows_.add(
                new PluginWindow(p, t, active_plugin_windows_));
        }
    }

    return nullptr;
}

bool MenubarComponent::closeOpenPluginWindows()
{
    bool wasEmpty = active_plugin_windows_.isEmpty();
    active_plugin_windows_.clear();
    return !wasEmpty;
}
// End from JUCE AudioPluginHost

void MenubarComponent::handleOpenPlugin(juce::AudioProcessorGraph::Node* p)
{
  if (auto* w = getOrCreatePluginWindow(p, PluginWindow::Type::normal))
    w->toFront(true);
}

void MenubarComponent::addActivePluginsToMenu(juce::PopupMenu& popup_menu)
{
  for (auto& plugin : pm_.getActivePlugins())
  {
    auto active_plugin_options = std::make_unique<juce::PopupMenu>();
    active_plugin_options->addItem("Open",
                                   [this, &plugin] ()
                                   {
                                     if (plugin->plugin_node && // plugin loaded
                                         plugin->plugin_node->getProcessor()->hasEditor())
                                       handleOpenPlugin(plugin->plugin_node.get());
                                   }
    );
    active_plugin_options->addItem("Remove",
                                   [this, &plugin] ()
                                   {
                                     pm_.removePlugin(plugin.get());
                                   }
    );
//    active_plugin_options->addItem(plugin->bypassed ? "Enable" : "Disable",
//                                   [&plugin] ()
//                                   {
//                                     // plugin->bypassed = !plugin->bypassed; // TODO
//                                   }
//    );
    
    auto plugin_item = PopupMenu::Item(plugin->description.name);
    // plugin_item.isEnabled = !plugin->bypassed;
    plugin_item.subMenu = std::move(active_plugin_options);

    popup_menu.addItem(plugin_item);
  }
}
