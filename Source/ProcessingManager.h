/*
  ==============================================================================

    PluginManager.h
    Created: 14 Jan 2022 3:46:34am
    Author:  Andrew Orals

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// Create a plugin selector class
// Create plugin editor class
// both these windows will be change listeners for the active and available plugins lists

using namespace juce;
using NodeID = juce::AudioProcessorGraph::NodeID;

struct PluginData
{
  PluginDescription                    description;
  std::unique_ptr<AudioPluginInstance> instance;
};

class ProcessingManager : public ChangeListener
{
public:
  ProcessingManager(juce::AudioDeviceManager& device_manager);
  ~ProcessingManager();
//======core======
  juce::String initialize();
  void addPlugin(PluginDescription& plugin);
  void insertPlugin(PluginDescription& plugin, unsigned int idx);
  void removePlugin(PluginDescription& plugin);
//======callbacks======
  void changeListenerCallback(ChangeBroadcaster* source) override;
//======getters======
  AudioPluginFormatManager& getPluginFormatManager();
  KnownPluginList& getAvailablePlugins();
  const Array<PluginData*>& getActivePlugins() const;
private:
  void loadActivePlugins(bool reload_all);
  void addActivePluginsToGraph();
  bool connectNodeToNode(AudioProcessorGraph::NodeID from, int from_chans,
                         AudioProcessorGraph::NodeID to, int to_chans);
  using NodePtr = AudioProcessorGraph::Node::Ptr;
  //======plugins======
  AudioPluginFormatManager format_manager_;
  KnownPluginList available_plugins_;
  Array<PluginData*> active_plugins_order_;
  Array<PluginData> active_plugins_;
  static const auto kPluginSortMethod =
  KnownPluginList::SortMethod::sortByManufacturer;
  static const auto kNumPluginLoadingThreads = 4;
  static const auto kPluginLoadingTimeoutMS  = 1000;
  //======processing graph======
  AudioDeviceManager& device_manager_;
  AudioProcessorGraph graph_;
  AudioProcessorPlayer player_;
  const NodeID kInputID = NodeID(0x0);
  const NodeID kOutputID = NodeID(0xffffffff);
};
