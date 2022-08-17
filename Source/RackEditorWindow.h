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
#include <functional>
#include <list>
#include <string>

#include "ProcessingManager.h"

class RackEditorWindow;
typedef std::function<void (std::pair<unsigned int, unsigned int>)> DragDropCallback;
constexpr const char PLUGIN_DRAG_SOURCE_ID[] = "PluginRowDragSource";

//==============================================================================

class RackEntry : public juce::Component,
                  public juce::DragAndDropTarget
{
public:
  RackEntry(const juce::String& text,
            int row_num,
            int height,
            int width,
            bool selected,
            const DragDropCallback& drag_drop_callback);

  ~RackEntry() override;

  virtual bool isInterestedInDragSource(
      const SourceDetails& dragSourceDetails) override;

  virtual void itemDropped(const SourceDetails& dragSourceDetails) override;

  virtual void paint(Graphics& g) override;

private:
  juce::String text_;
  const int row_num_;
  const int height_;
  const int width_;
  const DragDropCallback drag_drop_callback_;
};

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

  virtual juce::var getDragSourceDescription(
      const juce::SparseSet<int>& currentlySelectedRows) override;


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


class RackEditorWindow : public juce::DocumentWindow,
                         public juce::DragAndDropContainer
{
public:
  RackEditorWindow(std::list<std::unique_ptr<PluginData>>& active_plugin_order);
  ~RackEditorWindow() override;

  void closeButtonPressed() override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackEditorWindow)
  std::unique_ptr<PluginRack> table_list_model_;
  static constexpr int HEADER_HEIGHT = 22;
  static constexpr int ROW_HEIGHT = 20;
};
