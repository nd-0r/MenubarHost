/*
  ==============================================================================

    PluginManager.cpp
    Created: 14 Jan 2022 3:46:34am
    Author:  Andrew Orals

  ==============================================================================
*/

#include "ProcessingManager.h"

ProcessingManager::ProcessingManager(ApplicationProperties& app_properties,
                                     juce::AudioDeviceManager& device_manager) :
    device_manager_(device_manager), app_properties_(app_properties)
{
  format_manager_.addDefaultFormats();
  available_plugins_.setCustomScanner(std::make_unique<CustomPluginScanner>());
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

void ProcessingManager::addPlugin(unsigned int plugin_idx) {
  if (plugin_idx >= available_plugins_.getNumTypes())
    return;

  PluginDescription plugin = available_plugins_.getTypes()[plugin_idx];
  active_plugins_.add({plugin, nullptr});
  active_plugins_order_.add(
      active_plugins_.getRawDataPointer() + active_plugins_.size() - 1
  );
  loadActivePlugins(false);
}

void ProcessingManager::insertPlugin(unsigned int plugin_idx,
                                     unsigned int stack_idx)
{
  if (plugin_idx >= available_plugins_.getNumTypes() ||
      stack_idx >= active_plugins_.size())
    return;
 
  PluginDescription plugin = available_plugins_.getTypes()[plugin_idx];
  active_plugins_.add({plugin, nullptr});
  active_plugins_order_.insert(
      stack_idx,
      active_plugins_.getRawDataPointer() + active_plugins_.size() - 1
  );
  loadActivePlugins(false);
}

void ProcessingManager::removePlugin(PluginDescription& plugin) {
  PluginData const* loc = nullptr;
  for (unsigned int idx = 0; idx < active_plugins_.size(); ++idx)
  {
    PluginData& pd = active_plugins_.getReference(idx);
    if (pd.description.uniqueId == plugin.uniqueId)
    {
      graph_.removeNode(pd.plugin_node.get());
      pd.plugin_node = nullptr;
      loc = &pd;
      break;
    }
  }
 
  if (!loc)
    return;
 
  active_plugins_order_.removeIf([&loc] (PluginData* elem) {
    return elem == loc;
  });
  loadActivePlugins(false);
}

void ProcessingManager::changeListenerCallback(ChangeBroadcaster* source) {
  if (source == &available_plugins_)
  {
    std::unique_ptr<XmlElement> saved_plugin_list =
        available_plugins_.createXml();
    if (saved_plugin_list)
    {
      app_properties_.getUserSettings()->setValue(
          "pluginList", saved_plugin_list.get()
      );
      app_properties_.saveIfNeeded();
    }
  }
}

AudioPluginFormatManager& ProcessingManager::getPluginFormatManager()
{
  return format_manager_;
}

KnownPluginList& ProcessingManager::getAvailablePlugins()
{
  return available_plugins_;
}

const Array<PluginData*, CriticalSection> ProcessingManager::getActivePlugins() const
{
  return active_plugins_order_;
}

//======private======

void ProcessingManager::loadActivePlugins(bool reload_all)
{
  pthread_mutex_lock(&condvar_mutex);
  num_processed_plugins = active_plugins_.size();
  Logger::getCurrentLogger()->outputDebugString(std::string("Num loaded plugins: ") + std::to_string(num_processed_plugins));
  pthread_mutex_unlock(&condvar_mutex);

  for (auto& plugin_data : active_plugins_)
  {
    if (!plugin_data.plugin_node || reload_all)
    {
      pthread_mutex_lock(&condvar_mutex);
      --num_processed_plugins;
      Logger::getCurrentLogger()->outputDebugString(std::string("Decrementing loaded plugins: ") + std::to_string(num_processed_plugins));
      pthread_mutex_unlock(&condvar_mutex);

      format_manager_.createPluginInstanceAsync(
          plugin_data.description,
          graph_.getSampleRate(),
          graph_.getBlockSize(),
          [this, &plugin_data] (std::unique_ptr<AudioPluginInstance> instance,
                                const juce::String& error)
          {
            Logger::getCurrentLogger()->outputDebugString(std::string("Loading plugin: ") + plugin_data.description.name.toStdString());
            pluginCreateCallback(std::move(instance), plugin_data, error);
          }
      );
    }
  }
 
  Thread::launch([this] (void) {
    addActivePluginsToGraph();
  });
}

void ProcessingManager::pluginCreateCallback(std::unique_ptr<AudioPluginInstance> instance,
                                             PluginData& plugin_data,
                                             const juce::String& error)
{
  Logger::getCurrentLogger()->outputDebugString(std::string("Plugin create callback: ") + plugin_data.description.name.toStdString());
  if (!instance)
  {
    Logger::getCurrentLogger()->outputDebugString(std::string("Create plugin failed: ") + plugin_data.description.name.toStdString());
    AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,
                                     "Could not create plugin instance",
                                     error);
    plugin_data.plugin_node = nullptr;
    pthread_cond_signal(&plugin_load_condvar);
  }
  else
  {
    Logger::getCurrentLogger()->outputDebugString(std::string("Create plugin succeeded: ") + plugin_data.description.name.toStdString());
    instance->enableAllBuses();
    plugin_data.plugin_node = graph_.addNode(std::move(instance));
 
    pthread_mutex_lock(&condvar_mutex);
    ++num_processed_plugins;
    pthread_mutex_unlock(&condvar_mutex);
 
    pthread_cond_signal(&plugin_load_condvar);
  }
}

void ProcessingManager::addActivePluginsToGraph() {
  pthread_mutex_lock(&condvar_mutex);
  while (num_processed_plugins != active_plugins_.size())
    pthread_cond_wait(&plugin_load_condvar, &condvar_mutex);
  pthread_mutex_unlock(&condvar_mutex);

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
      PluginData& active_plugin_data = *active_plugins_order_[idx];
      if (active_plugin_data.plugin_node && !active_plugin_data.bypassed)
      {
        NodeID curr_id = active_plugin_data.plugin_node->nodeID;
        int input_chans =
            active_plugin_data.plugin_node->
                getProcessor()->getChannelCountOfBus(true /* in */, 0);
        connectNodeToNode(prev, prev_chans, curr_id, input_chans); // TODO handle error

        prev = curr_id;
        prev_chans =
            active_plugin_data.plugin_node->
                getProcessor()->getChannelCountOfBus(false /* out */, 0); // TODO handle error
      }
    }
    connectNodeToNode(prev, prev_chans, kOutputID, num_outputs); // TODO handle error
  }
}

bool ProcessingManager::connectNodeToNode(AudioProcessorGraph::NodeID from,
                                          int from_chans,
                                          AudioProcessorGraph::NodeID to,
                                          int to_chans)
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
