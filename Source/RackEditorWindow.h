/*
  ==============================================================================

    RackEditorWindow.h
    Author:  Andrew Orals

  ==============================================================================
*/

/*
 Reorder, add, insert, and remove plugins here
 Save plugin order in savefile
 */

#pragma once

#include <JuceHeader.h>
#include <list>

#include "ProcessingManager.h"

class RackEditorWindow;

//==============================================================================

class PluginRack : public juce::TableListBoxModel
{
public:
  PluginRack(const RackEditorWindow& owner,
             std::list<std::unique_ptr<PluginData>>& active_plugin_order);
  ~PluginRack() override;

  virtual int getNumRows() override;
  virtual void paintRowBackground(juce::Graphics& g,
                                  int rowNumber,
                                  int width,
                                  int height,
                                  bool rowIsSelected) override;
  virtual void paintCell(juce::Graphics& g,
                         int rowNumber,
                         int columnId,
                         int width,
                         int height,
                         bool rowIsSelected) override;

  enum
  {
    idCol = 1,
    nameCol = 2,
    typeCol = 3,
    categoryCol = 4,
    manufacturerCol = 5,
  };

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginRack)
  const RackEditorWindow& owner_;
  std::list<std::unique_ptr<PluginData>>& active_plugin_order_;
};


class RackEditorWindow : public juce::DocumentWindow
{
public:
  RackEditorWindow(std::list<std::unique_ptr<PluginData>>& active_plugin_order);
  ~RackEditorWindow() override;

  void closeButtonPressed() override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackEditorWindow)
  std::unique_ptr<PluginRack> table_list_model_;
};
