/*
  ==============================================================================

    PluginManager.cpp
    Created: 14 Jan 2022 3:46:34am
    Author:  Andrew Orals

  ==============================================================================
*/

/*
FIXES:
 - close window and cleanup when plugin removed
 - connect midi to all plugins which accept midi input
   - add option to chain midi

TODO:
 - broadcast audio output
 - integrated plugin scanning (menubar item)
 - reorder plugins UI
 - save window preferences globally
 - savefile
   - plugin state
   - active plugins
   - audio/midi device configuration
 - test MIDI keyboard
 - make menu not disappear when adding plugins
 - menu option to mute input
 - menu option to reload plugins
 - unit tests

 - semver
 - linting
 - CI/CD
 */

#include "ProcessingManager.h"

ProcessingManager::ProcessingManager(ApplicationProperties& app_properties,
                                     juce::AudioDeviceManager& device_manager) :
    device_manager_(device_manager), app_properties_(app_properties)
{
  format_manager_.addDefaultFormats();

  auto plugin_list_xml =
      app_properties_.getUserSettings()->getXmlValue("pluginList");
  if (plugin_list_xml != nullptr)
    available_plugins_.recreateFromXml(*plugin_list_xml);
  available_plugins_.setCustomScanner(std::make_unique<CustomPluginScanner>());
  available_plugins_.addChangeListener(this);
  
  initialize();

  // device_manager_.addChangeListener(graphPanel.get());
  // device_manager_.addChangeListener(this);
}

ProcessingManager::~ProcessingManager()
{
  auto midiDevice = juce::MidiInput::getDefaultDevice();
  device_manager_.setMidiInputDeviceEnabled(midiDevice.identifier, false);
  device_manager_.removeMidiInputDeviceCallback(midiDevice.identifier, &player_);
  device_manager_.removeAudioCallback(&player_);
  device_manager_.closeAudioDevice();
}

void ProcessingManager::initialize() {
  player_.setProcessor(&graph_);
  player_.getCurrentProcessor()->enableAllBuses();
  graph_.addNode(
      std::make_unique<AudioProcessorGraph::AudioGraphIOProcessor>(
          AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode
      ),
      kAudioInputID
  )->getProcessor()->enableAllBuses();
  graph_.addNode(
      std::make_unique<AudioProcessorGraph::AudioGraphIOProcessor>(
          AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode
      ),
      kAudioOutputID
  )->getProcessor()->enableAllBuses();
  graph_.addNode(
      std::make_unique<AudioProcessorGraph::AudioGraphIOProcessor>(
          AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode
      ),
      kMidiInputID
  );
  graph_.addNode(
      std::make_unique<AudioProcessorGraph::AudioGraphIOProcessor>(
          AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode
      ),
      kMidiOutputID
  );
  
  auto midiInputDevice = juce::MidiInput::getDefaultDevice();
  auto midiOutputDevice = juce::MidiOutput::getDefaultDevice();
  
  device_manager_.setMidiInputDeviceEnabled(midiInputDevice.identifier, true);
  device_manager_.addMidiInputDeviceCallback(midiInputDevice.identifier, &player_);
  device_manager_.setDefaultMidiOutputDevice(midiOutputDevice.identifier);

  // From JUCE AudioPluginHost
  // https://github.com/juce-framework/JUCE/blob/master/extras/AudioPluginHost/Source/UI/MainHostWindow.cpp
  RuntimePermissions::request(
      RuntimePermissions::recordAudio,
      [this] (bool granted) mutable
      {
        auto savedState =
            app_properties_.getUserSettings()->getXmlValue ("audioDeviceState");
        device_manager_.initialise(granted ? 256 : 0,
                                   256,
                                   savedState.get(),
                                   true);
        device_manager_.addAudioCallback(&player_);
        // device_manager_.addMidiInputDeviceCallback({}, &graphPlayer.getMidiMessageCollector());
        loadActivePlugins(false);
      }
  );
  // End from JUCE AudioPluginHost
}

void ProcessingManager::addPlugin(unsigned int plugin_idx) {
  if (plugin_idx >= available_plugins_.getNumTypes())
    return;
  
  PluginDescription plugin = available_plugins_.getTypes()[plugin_idx];
  active_plugins_ordered_.push_back(std::make_unique<PluginData>(plugin, nullptr));
  loadActivePlugins(false);
}

void ProcessingManager::insertPlugin(unsigned int plugin_idx,
                                     unsigned int stack_idx)
{
  if (plugin_idx >= available_plugins_.getNumTypes() ||
      stack_idx >= active_plugins_ordered_.size())
    return;
 
  PluginDescription plugin = available_plugins_.getTypes()[plugin_idx];
  active_plugins_ordered_.push_back(std::make_unique<PluginData>(plugin, nullptr));
  loadActivePlugins(false);
}

void ProcessingManager::removePlugin(const PluginData* plugin) {
  for (auto iter = active_plugins_ordered_.begin();
       iter != active_plugins_ordered_.end();
       ++iter)
  {
    auto& pd = *iter;
    if (pd.get() == plugin)
    {
      graph_.removeNode(pd->plugin_node.get());
      pd->plugin_node = nullptr;
      active_plugins_ordered_.erase(iter);
      break;
    }
  }
 
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

std::list<std::unique_ptr<PluginData>>& ProcessingManager::getActivePlugins()
{
  return active_plugins_ordered_;
}

//======private======

void ProcessingManager::loadActivePlugins(bool reload_all)
{
  pthread_mutex_lock(&condvar_mutex);
  num_processed_plugins = active_plugins_ordered_.size();
  Logger::getCurrentLogger()->outputDebugString(std::string("Num loaded plugins: ") + std::to_string(num_processed_plugins));
  pthread_mutex_unlock(&condvar_mutex);

  for (auto& plugin_data : active_plugins_ordered_)
  {
    if (!plugin_data->plugin_node || reload_all)
    {
      pthread_mutex_lock(&condvar_mutex);
      --num_processed_plugins;
      Logger::getCurrentLogger()->outputDebugString(std::string("Decrementing loaded plugins: ") + std::to_string(num_processed_plugins));
      pthread_mutex_unlock(&condvar_mutex);

      format_manager_.createPluginInstanceAsync(
          plugin_data->description,
          graph_.getSampleRate(),
          graph_.getBlockSize(),
          [this, &plugin_data] (std::unique_ptr<AudioPluginInstance> instance,
                                const juce::String& error)
          {
            Logger::getCurrentLogger()->outputDebugString(std::string("Loading plugin: ") + plugin_data->description.name.toStdString());
            pluginCreateCallback(std::move(instance), *plugin_data, error);
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
  }
  else if (curr_node_uid <= kMaxPluginUid)
  {
    Logger::getCurrentLogger()->outputDebugString(std::string("Create plugin succeeded: ") + plugin_data.description.name.toStdString());
    instance->enableAllBuses();
    plugin_data.plugin_node = graph_.addNode(std::move(instance),
                                             NodeID(curr_node_uid++));
    Logger::getCurrentLogger()->outputDebugString(std::string("Plugin node pointer: ") + std::to_string((long) plugin_data.plugin_node.get()));
 
    pthread_mutex_lock(&condvar_mutex);
    ++num_processed_plugins;
    pthread_mutex_unlock(&condvar_mutex);
  }
  else
  {
    AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,
                                     "Congratulations",
                                     "Congratulations, you've added 4,294,967,294 plugins !");
    plugin_data.plugin_node = nullptr;
  }
  pthread_cond_signal(&plugin_load_condvar);
}

void ProcessingManager::addActivePluginsToGraph() {
  const auto midiChannelIndex = juce::AudioProcessorGraph::midiChannelIndex;
  pthread_mutex_lock(&condvar_mutex);
  while (num_processed_plugins != active_plugins_ordered_.size())
    pthread_cond_wait(&plugin_load_condvar, &condvar_mutex);
  pthread_mutex_unlock(&condvar_mutex);
  auto* logger = Logger::getCurrentLogger();

  logger->outputDebugString(std::string("Connecting graph nodes"));

  for (auto& connection : graph_.getConnections())
    graph_.removeConnection(connection);
 
  auto* audio_device = device_manager_.getCurrentAudioDevice();
  const int num_inputs = audio_device->getActiveInputChannels().toInteger();
  const int num_outputs = audio_device->getActiveOutputChannels().toInteger();
  
  int prev_chans = num_inputs;
  NodeID prev_audio = kAudioInputID;
  NodeID prev_midi = kMidiInputID;
  
  if (active_plugins_ordered_.empty())
  {
    logger->outputDebugString("Connecting input direct to output");
    connectNodeToNode(kAudioInputID, kAudioOutputID, 0, num_inputs, 0, num_outputs); // TODO handle error
    connectNodeToNode(kMidiInputID, kMidiOutputID,
                      midiChannelIndex, midiChannelIndex + 1,
                      midiChannelIndex, midiChannelIndex + 1);
  }
  else
  {
    for (auto iter = active_plugins_ordered_.begin();
         iter != active_plugins_ordered_.end();
         ++iter)
    {
      const auto& active_plugin_data = *iter;

      if (active_plugin_data->plugin_node /* && !active_plugin_data.bypassed */)
      {
        NodeID curr_id = active_plugin_data->plugin_node->nodeID;
        const auto processor = active_plugin_data->plugin_node->getProcessor();
 
        logger->outputDebugString("CurrID: " + std::to_string(curr_id.uid));
 
        int input_chans = processor->getChannelCountOfBus(true /* in */, 0);
        connectNodeToNode(prev_audio, curr_id, 0, prev_chans, 0, input_chans); // TODO handle error
        
        if (processor->acceptsMidi())
        {
          logger->outputDebugString("Connecting MIDI!!!: ");
          connectNodeToNode(prev_midi, curr_id,
                            midiChannelIndex, midiChannelIndex + 1,
                            midiChannelIndex, midiChannelIndex + 1);
        }

        prev_audio = curr_id;
        prev_midi = processor->producesMidi() ? curr_id : prev_midi;
        // TODO what if something produces but doesn't accept midi?
        prev_chans = processor->getChannelCountOfBus(false /* out */, 0); // TODO handle error
      }
    }
    connectNodeToNode(prev_audio, kAudioOutputID, 0, prev_chans, 0, num_outputs); // TODO handle error
    connectNodeToNode(prev_midi, kMidiOutputID,
                      midiChannelIndex, midiChannelIndex + 1,
                      midiChannelIndex, midiChannelIndex + 1);
  }

  if (graph_.removeIllegalConnections())
    logger->outputDebugString("BAD! Had to remove illegal connections");
  
  std::vector<AudioProcessorGraph::Connection> connections = graph_.getConnections();
  int idx = 0;
  for (auto& conn : connections)
  {
    logger->outputDebugString( "======= CONNECTION: " + std::to_string(idx) + " =======");
    logger->outputDebugString( "Is MIDI: " + std::to_string(conn.source.isMIDI()));
    logger->outputDebugString( "Source name: " + graph_.getNodeForId(conn.source.nodeID)->getProcessor()->getName());
    logger->outputDebugString( "Source node ID: " + std::to_string(conn.source.nodeID.uid));
    logger->outputDebugString( "Source channel idx: " + std::to_string(conn.source.channelIndex));
    logger->outputDebugString( "Destination name: " + graph_.getNodeForId(conn.destination.nodeID)->getProcessor()->getName());
    logger->outputDebugString( "Destination node ID: " + std::to_string(conn.destination.nodeID.uid));
    logger->outputDebugString( "Destination channel idx: " + std::to_string(conn.destination.channelIndex));
    logger->outputDebugString( "======= END CONNECTION: " + std::to_string(idx++) + " =======");
    
  }
}

bool ProcessingManager::connectNodeToNode(AudioProcessorGraph::NodeID from,
                                          AudioProcessorGraph::NodeID to,
                                          int from_chans_min,
                                          int from_chans_max, // exclusive
                                          int to_chans_min,
                                          int to_chans_max) // exclusive
{
  Logger::getCurrentLogger()->outputDebugString("Connecting " + std::to_string(from.uid) + " to " + std::to_string(to.uid));
  for (int channel_idx = juce::jmax(from_chans_min, to_chans_min);
       channel_idx < juce::jmin(from_chans_max, to_chans_max);
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
