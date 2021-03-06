/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

using namespace juce;

constexpr const char* processUID = "juceaudiopluginhost";
constexpr const char* scanModeKey = "pluginScanMode";

class CustomPluginScanner  : public KnownPluginList::CustomScanner,
                             private ChangeListener
{
 public:
  CustomPluginScanner()
  {
//    if (auto* file = getAppProperties().getUserSettings())
//      file->addChangeListener (this);

    changeListenerCallback (nullptr);
  }

  ~CustomPluginScanner() override
  {
//    if (auto* file = getAppProperties().getUserSettings())
//      file->removeChangeListener (this);
  }

  bool findPluginTypesFor (AudioPluginFormat& format,
                           OwnedArray<PluginDescription>& result,
                           const String& fileOrIdentifier) override
  {
    if (scanInProcess)
    {
      superprocess = nullptr;
      format.findAllTypesForFile (result, fileOrIdentifier);
      return true;
    }

    if (superprocess == nullptr)
    {
      superprocess = std::make_unique<Superprocess> (*this);

      std::unique_lock<std::mutex> lock (mutex);
      connectionLost = false;
    }

    MemoryBlock block;
    MemoryOutputStream stream { block, true };
    stream.writeString (format.getName());
    stream.writeString (fileOrIdentifier);

    if (superprocess->sendMessageToWorker (block))
    {
      std::unique_lock<std::mutex> lock (mutex);
      gotResponse = false;
      pluginDescription = nullptr;

      for (;;)
      {
        if (condvar.wait_for (lock,
                    std::chrono::milliseconds (50),
                    [this] { return gotResponse || shouldExit(); }))
        {
          break;
        }
      }

      if (shouldExit())
      {
        superprocess = nullptr;
        return true;
      }

      if (connectionLost)
      {
        superprocess = nullptr;
        return false;
      }

      if (pluginDescription != nullptr)
      {
        for (const auto* item : pluginDescription->getChildIterator())
        {
          auto desc = std::make_unique<PluginDescription>();

          if (desc->loadFromXml (*item))
            result.add (std::move (desc));
        }
      }

      return true;
    }

    superprocess = nullptr;
    return false;
  }

  void scanFinished() override
  {
    superprocess = nullptr;
  }

 private:
  class Superprocess  : private ChildProcessCoordinator
  {
   public:
    explicit Superprocess (CustomPluginScanner& o) :
        owner (o)
    {
      launchWorkerProcess (File::getSpecialLocation (File::currentExecutableFile), processUID, 0, 0);
    }

    using ChildProcessCoordinator::sendMessageToWorker;

   private:
    void handleMessageFromWorker (const MemoryBlock& mb) override
    {
      auto xml = parseXML (mb.toString());

      const std::lock_guard<std::mutex> lock (owner.mutex);
      owner.pluginDescription = std::move (xml);
      owner.gotResponse = true;
      owner.condvar.notify_one();
    }

    void handleConnectionLost() override
    {
      const std::lock_guard<std::mutex> lock (owner.mutex);
      owner.pluginDescription = nullptr;
      owner.gotResponse = true;
      owner.connectionLost = true;
      owner.condvar.notify_one();
    }

    CustomPluginScanner& owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Superprocess)
  };

  void changeListenerCallback (ChangeBroadcaster*) override
  {
//    if (auto* file = getAppProperties().getUserSettings())
//      scanInProcess = (file->getIntValue (scanModeKey) == 0);
  }

  std::unique_ptr<Superprocess> superprocess;
  std::mutex mutex;
  std::condition_variable condvar;
  std::unique_ptr<XmlElement> pluginDescription;
  bool gotResponse = false;
  bool connectionLost = false;

  std::atomic<bool> scanInProcess { true };

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomPluginScanner)
};
