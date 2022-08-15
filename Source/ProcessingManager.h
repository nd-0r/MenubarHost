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
#include <list>
#include "CustomPluginScanner.hpp"


using namespace juce;
using NodeID = juce::AudioProcessorGraph::NodeID;

class MenubarHostApplication;

struct PluginData
{
  PluginData(PluginDescription& plug_desc,
             AudioProcessorGraph::Node::Ptr node) :
      description(plug_desc), plugin_node(node) { }
  PluginDescription                    description;
  AudioProcessorGraph::Node::Ptr       plugin_node;
};

class ProcessingManager : public ChangeListener
{
public:
  ProcessingManager(ApplicationProperties& app_properties,
                    juce::AudioDeviceManager& device_manager);
  ~ProcessingManager();

  void initialize();
  void addPlugin(unsigned int plugin_idx);
  void insertPlugin(unsigned int plugin_idx, unsigned int stack_idx);
  void removePlugin(const PluginData* plugin);

  void changeListenerCallback(ChangeBroadcaster* source) override;

  AudioPluginFormatManager& getPluginFormatManager();
  KnownPluginList& getAvailablePlugins();
  std::list<std::unique_ptr<PluginData>>& getActivePlugins();
private:
  void loadActivePlugins(bool reload_all);
  void pluginCreateCallback(std::unique_ptr<AudioPluginInstance> instance,
                            PluginData& plugin_data,
                            const juce::String& error);
  void addActivePluginsToGraph();
  bool connectNodeToNode(AudioProcessorGraph::NodeID from,
                         AudioProcessorGraph::NodeID to,
                         int from_chans_min,
                         int from_chans_max,
                         int to_chans_min,
                         int to_chans_max);
  using NodePtr = AudioProcessorGraph::Node::Ptr;
  //======plugins======
  AudioPluginFormatManager format_manager_;
  KnownPluginList available_plugins_;
  std::list<std::unique_ptr<PluginData>> active_plugins_ordered_;
  static const auto kPluginSortMethod =
      KnownPluginList::SortMethod::sortByManufacturer;
  static const auto kNumPluginLoadingThreads = 4;
  static const auto kPluginLoadingTimeoutMS  = 100000;
  //======synchronization======
  pthread_cond_t plugin_load_condvar;
  pthread_mutex_t condvar_mutex;
  pthread_mutex_t active_plugin_list_mutex;
  size_t num_processed_plugins = 0;
  //======processing graph======
  AudioDeviceManager& device_manager_;
  AudioProcessorGraph graph_;
  AudioProcessorPlayer player_;
  const uint32 kMaxPluginUid = 0xfffffffb;
  const NodeID kAudioInputID = NodeID(kMaxPluginUid + 1);
  const NodeID kAudioOutputID = NodeID(kMaxPluginUid + 2);
  const NodeID kMidiInputID = NodeID(kMaxPluginUid + 3);
  const NodeID kMidiOutputID = NodeID(kMaxPluginUid + 4);
  uint32 curr_node_uid = 0x0;
  //======state references======
  ApplicationProperties& app_properties_;
};
