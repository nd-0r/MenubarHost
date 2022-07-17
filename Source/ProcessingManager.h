/*
  ==============================================================================

    PluginManager.h
    Created: 14 Jan 2022 3:46:34am
    Author:  Andrew Orals

  ==============================================================================
*/

#pragma once
#include <pthread.h>
#include <JuceHeader.h>
#include "CustomPluginScanner.hpp"


using namespace juce;
using NodeID = juce::AudioProcessorGraph::NodeID;

class MenubarHostApplication;

struct PluginData
{
  PluginDescription                    description;
  AudioProcessorGraph::Node::Ptr       plugin_node;
  bool bypassed = false;
};

class ProcessingManager : public ChangeListener
{
public:
  ProcessingManager(ApplicationProperties& app_properties,
                    juce::AudioDeviceManager& device_manager);
  ~ProcessingManager();

  juce::String initialize();
  void addPlugin(unsigned int plugin_idx);
  void insertPlugin(unsigned int plugin_idx, unsigned int stack_idx);
  void removePlugin(PluginDescription& plugin);

  void changeListenerCallback(ChangeBroadcaster* source) override;

  AudioPluginFormatManager& getPluginFormatManager();
  KnownPluginList& getAvailablePlugins();
  const Array<PluginData*, CriticalSection> getActivePlugins() const;
private:
  void loadActivePlugins(bool reload_all);
  void pluginCreateCallback(std::unique_ptr<AudioPluginInstance> instance,
                            PluginData& plugin_data,
                            const juce::String& error);
  void addActivePluginsToGraph();
  bool connectNodeToNode(AudioProcessorGraph::NodeID from, int from_chans,
                         AudioProcessorGraph::NodeID to, int to_chans);
  using NodePtr = AudioProcessorGraph::Node::Ptr;
  //======plugins======
  AudioPluginFormatManager format_manager_;
  KnownPluginList available_plugins_;
  Array<PluginData*, CriticalSection> active_plugins_order_;
  Array<PluginData, CriticalSection> active_plugins_;
  static const auto kPluginSortMethod =
      KnownPluginList::SortMethod::sortByManufacturer;
  static const auto kNumPluginLoadingThreads = 4;
  static const auto kPluginLoadingTimeoutMS  = 100000;
  //======synchronization======
  pthread_cond_t plugin_load_condvar;
  pthread_mutex_t condvar_mutex;
  unsigned int num_processed_plugins = 0;
  //======processing graph======
  AudioDeviceManager& device_manager_;
  AudioProcessorGraph graph_;
  AudioProcessorPlayer player_;
  const NodeID kInputID = NodeID(0x0);
  const NodeID kOutputID = NodeID(0xffffffff);
  //======state references======
  ApplicationProperties& app_properties_;
};
