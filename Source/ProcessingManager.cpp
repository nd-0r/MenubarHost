/*
  ==============================================================================

    PluginManager.cpp
    Created: 14 Jan 2022 3:46:34am
    Author:  Andrew Orals

  ==============================================================================
*/

#include "ProcessingManager.h"

ProcessingManager::ProcessingManager(juce::AudioDeviceManager& device_manager) :
    device_manager_(device_manager)
{
  // Nothing
}

ProcessingManager::~ProcessingManager()
{
  device_manager_.removeAudioCallback(&player_);
  device_manager_.closeAudioDevice();
}

juce::String ProcessingManager::initialize() {
  juce::String err_str = device_manager_.initialiseWithDefaultDevices(2, 2);
  player_.setProcessor(&graph_);
  device_manager_.addAudioCallback(&player_);
  addActivePluginsToGraph();
  return err_str;
}

void ProcessingManager::addPlugin(PluginDescription& plugin) {
  active_plugins_.add({plugin, nullptr});
  active_plugins_order_.add(
      active_plugins_.getRawDataPointer() + active_plugins_.size() - 1
  );
}

void ProcessingManager::insertPlugin(PluginDescription& plugin,
                                     unsigned int idx)
{
  if (idx >= active_plugins_.size())
    return;
 
  active_plugins_.add({plugin, nullptr});
  active_plugins_order_.insert(
      idx, active_plugins_.getRawDataPointer() + active_plugins_.size() - 1
  );
}

void ProcessingManager::removePlugin(PluginDescription& plugin) {
  PluginData const* loc = nullptr;
  active_plugins_.removeIf([&plugin, &loc] (const PluginData& pd) {
    if (pd.description.uniqueId == plugin.uniqueId)
    {
      loc = &pd;
      return true;
    }
    return false;
  });
  
  if (!loc)
    return;
 
  active_plugins_order_.removeIf([&loc] (const PluginData* pd) {
    return pd == loc;
  });
}

void ProcessingManager::changeListenerCallback(ChangeBroadcaster* source) {
  Logger::outputDebugString("NOT IMPLEMENTED");
}

AudioPluginFormatManager& ProcessingManager::getPluginFormatManager()
{
  return format_manager_;
}

KnownPluginList& ProcessingManager::getAvailablePlugins()
{
  return available_plugins_;
}

const Array<PluginData*>& ProcessingManager::getActivePlugins() const
{
  return active_plugins_order_;
}

//======private======

void ProcessingManager::loadActivePlugins(bool reload_all)
{
  auto tp = ThreadPool(kNumPluginLoadingThreads);
  for (auto& plugin_data : active_plugins_)
  {
    if (!plugin_data.instance || reload_all)
    {
      tp.addJob([&]() {
        juce::String error;
        plugin_data.instance =
            format_manager_.createPluginInstance(plugin_data.description,
                                                 graph_.getSampleRate(),
                                                 graph_.getBlockSize(),
                                                 error);
        if (!plugin_data.instance)
          Logger::outputDebugString(error);
      });
    }
  }
  tp.removeAllJobs(false, kPluginLoadingTimeoutMS);
}

void ProcessingManager::addActivePluginsToGraph() {
  graph_.clear();
  graph_.addNode(std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor>(
      new AudioProcessorGraph::AudioGraphIOProcessor(
          AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode)),
      kInputID
  );
  graph_.addNode(std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor>(
      new AudioProcessorGraph::AudioGraphIOProcessor(
          AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode)),
      kOutputID
  );
  
  auto* audio_device = device_manager_.getCurrentAudioDevice();
  const int num_inputs = audio_device->getActiveInputChannels().toInteger();
  const int num_outputs = audio_device->getActiveOutputChannels().toInteger();
  
  int prev_chans = num_inputs;
  NodeID prev = kInputID;
  
  jassert(active_plugins_.size() == active_plugins_order_.size());
  if (active_plugins_order_.isEmpty())
  {
    connectNodeToNode(kInputID, num_inputs, kOutputID, num_outputs); // TODO handle error
  }
  else
  {
    for (int idx = 0; idx < active_plugins_order_.size(); ++idx)
    {
      AudioPluginInstance* instance =
          active_plugins_order_[idx]->instance.get();
      if (instance)
      {
        NodeID curr_id = static_cast<NodeID>(idx + 1);
        graph_.addNode(std::unique_ptr<AudioProcessor>(instance), curr_id);
        int input_chans = instance->getChannelCountOfBus(true /*input*/, 0); // TODO handle error
        connectNodeToNode(prev, prev_chans, curr_id, input_chans); // TODO handle error
        prev = curr_id;
        prev_chans = instance->getChannelCountOfBus(false /*output*/, 0); // TODO handle error
      }
    }
    connectNodeToNode(prev, prev_chans, kOutputID, num_outputs); // TODO handle error
  }
}

bool ProcessingManager::connectNodeToNode(AudioProcessorGraph::NodeID from, int from_chans,
                                          AudioProcessorGraph::NodeID to, int to_chans)
{
  for (int channel_idx = 0;
       channel_idx < juce::jmin(from_chans, to_chans);
       ++channel_idx) {
    if (!graph_.addConnection(
            AudioProcessorGraph::Connection(
                {from, channel_idx}, {to, channel_idx}
            )
        ))
    {
      return false;
    }
  }
  return true;
}
