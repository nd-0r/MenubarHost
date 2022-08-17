/*
  ==============================================================================

    RackEditorWindow.cpp
    Author:  Andrew Orals

  ==============================================================================
*/

#include <JuceHeader.h>
#include "RackEditorWindow.h"

//==============================================================================


RackEntry::RackEntry(const juce::String& text,
          int row_num,
          int height,
          int width,
          bool selected,
          const DragDropCallback& drag_drop_callback) : 
    text_(text), row_num_(row_num), height_(height), width_(width),
    drag_drop_callback_(drag_drop_callback)
{
}

RackEntry::~RackEntry()
{
}

bool RackEntry::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
  return dragSourceDetails.sourceComponent != this &&
         dragSourceDetails.description == juce::var(PLUGIN_DRAG_SOURCE_ID);
}

void RackEntry::itemDropped(const SourceDetails& dragSourceDetails)
{
  jassert(isInterestedInDragSource(dragSourceDetails));

  const RackEntry* source = 
    static_cast<RackEntry*>(dragSourceDetails.sourceComponent.get());

  drag_drop_callback_({source->row_num_, row_num_});
}

void RackEntry::paint(Graphics& g)
{
  if (text_.isNotEmpty())
  {
    const auto defaultTextColour =
      owner_.findColour(juce::ListBox::textColourId);
    g.setColour (columnId == nameCol ?
                   defaultTextColour
                   :
                   defaultTextColour.interpolatedWith (juce::Colours::transparentBlack, 0.3f));
    g.setFont(juce::Font((float) height_ * 0.7f, juce::Font::bold));
    g.drawFittedText(
      text_, 4, 0, width_ - 6, height_, juce::Justification::centredLeft, 1, 0.9f);
  }
}

//==============================================================================

PluginRack::PluginRack(const RackEditorWindow& owner,
                       std::list<std::unique_ptr<PluginData>>& active_plugin_order) :
    owner_(owner),
    active_plugin_order_(active_plugin_order)
{
}

PluginRack::~PluginRack()
{
}

int PluginRack::getNumRows()
{
  return static_cast<int>(active_plugin_order_.size());
}

void PluginRack::paintRowBackground(juce::Graphics& g,
                                    int rowNumber,
                                    int width,
                                    int height,
                                    bool rowIsSelected)
{
  const auto defaultColour = owner_.findColour(juce::ListBox::backgroundColourId);
  const auto c = rowIsSelected ?
    defaultColour.interpolatedWith(
      owner_.findColour(juce::ListBox::textColourId), 0.5f)
    :
    defaultColour;

  g.fillAll (c);
}

void PluginRack::paintCell(juce::Graphics& g,
                           int rowNumber,
                           int columnId,
                           int width,
                           int height,
                           bool rowIsSelected)
{
  juce::String text = "";

  if (rowNumber < active_plugin_order_.size())
  {
    auto iter = active_plugin_order_.begin();
    std::advance(iter, rowNumber);
    auto& desc = (*iter)->description;

    switch (columnId)
    {
      case idCol:           text = juce::String((*iter)->plugin_node->nodeID.uid); break;
      case nameCol:         text = desc.name; break;
      case typeCol:         text = desc.pluginFormatName; break;
      case categoryCol:     text = desc.category.isNotEmpty() ? desc.category : "-"; break;
      case manufacturerCol: text = desc.manufacturerName; break;
      default: jassertfalse; break;
    }
  }

  // TODO
}


juce::var PluginRack::getDragSourceDescription(
    const juce::SparseSet<int>& currentlySelectedRows)
{
  return juce::var(PLUGIN_DRAG_SOURCE_ID);
}

//==============================================================================

RackEditorWindow::RackEditorWindow(
    std::list<std::unique_ptr<PluginData>>& active_plugin_order) :
  juce::DocumentWindow("Edit Plugin Configuration",
                       juce::Colours::grey,
                       DocumentWindow::TitleBarButtons::closeButton,
                       false)
{
  table_list_model_ = std::make_unique<PluginRack>(*this, active_plugin_order);
  auto* table_list_box = new juce::TableListBox("Edit Plugin Configuration",
                                                table_list_model_.get());
  auto& header = table_list_box->getHeader();

  header.addColumn("ID", PluginRack::idCol, 30, 30, 30, juce::TableHeaderComponent::notResizable);
  header.addColumn("Name", PluginRack::nameCol, 200, 100, 700, juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortedForwards);
  header.addColumn("Format", PluginRack::typeCol, 80, 80, 80,    juce::TableHeaderComponent::notResizable);
  header.addColumn("Category", PluginRack::categoryCol, 100, 100, 200);
  header.addColumn("Manufacturer", PluginRack::manufacturerCol, 200, 100, 300);

  table_list_box->setHeaderHeight(HEADER_HEIGHT);
  table_list_box->setRowHeight(ROW_HEIGHT);
  table_list_box->setMultipleSelectionEnabled(false);

  setSize(610, HEADER_HEIGHT + ROW_HEIGHT * static_cast<int>(active_plugin_order.size()));
  auto r = getLocalBounds().reduced(2);
  table_list_box->setBounds(r);

  setContentOwned(table_list_box, true);
  setUsingNativeTitleBar(true);
  setResizable(true, true);
  addToDesktop();
  setVisible(true);
}

RackEditorWindow::~RackEditorWindow()
{
}

void RackEditorWindow::closeButtonPressed()
{
  delete this;
}

