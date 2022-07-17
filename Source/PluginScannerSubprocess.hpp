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

class PluginScannerSubprocess : private ChildProcessWorker,
                                private AsyncUpdater
{
public:
  PluginScannerSubprocess()
  {
    formatManager.addDefaultFormats();
  }

  using ChildProcessWorker::initialiseFromCommandLine;

private:
  void handleMessageFromCoordinator (const MemoryBlock& mb) override
  {
    {
      const std::lock_guard<std::mutex> lock (mutex);
      pendingBlocks.emplace (mb);
    }

    triggerAsyncUpdate();
  }

  void handleConnectionLost() override
  {
    JUCEApplicationBase::quit();
  }

  // It's important to run the plugin scan on the main thread!
  void handleAsyncUpdate() override
  {
    for (;;)
    {
      const auto block = [&]() -> MemoryBlock
      {
        const std::lock_guard<std::mutex> lock (mutex);

        if (pendingBlocks.empty())
          return {};

        auto out = std::move (pendingBlocks.front());
        pendingBlocks.pop();
        return out;
      }();

      if (block.isEmpty())
        return;

      MemoryInputStream stream { block, false };
      const auto formatName = stream.readString();
      const auto identifier = stream.readString();

      OwnedArray<PluginDescription> results;

      for (auto* format : formatManager.getFormats())
        if (format->getName() == formatName)
          format->findAllTypesForFile (results, identifier);

      XmlElement xml ("LIST");

      for (const auto& desc : results)
        xml.addChildElement (desc->createXml().release());

      const auto str = xml.toString();
      sendMessageToCoordinator ({ str.toRawUTF8(), str.getNumBytesAsUTF8() });
    }
  }

  AudioPluginFormatManager formatManager;

  std::mutex mutex;
  std::queue<MemoryBlock> pendingBlocks;
};

