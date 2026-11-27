//
// Created by charles on 23/08/24.
//

#include <algorithm>

#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/StringUtils.hh"

namespace Ravl2
{
  bool OutputFormatMap::add(std::shared_ptr<OutputFormat> format)
  {
    auto extensions = splitStrings(format->extension(), ',');
    SPDLOG_TRACE("Adding format: '{}' Ext:'{}' ({}) Map:{}", format->name(), format->extension(), extensions.size(), static_cast<void *>(this));
    std::lock_guard lock(mMutex);
    auto addToExtension = [this, &format](const std::string &extension) {
      auto &formats = mFormatByExtension[toLowerCase(extension)];
      formats.push_back(format);
      std::stable_sort(formats.begin(), formats.end(), [](const auto &a, const auto &b) {
        return a->priority() > b->priority();
      });
    };
    if(extensions.empty()) {
      addToExtension("");
      return true;
    }
    for(const auto &ext : extensions) {
      addToExtension(ext);
    }
    return true;
  }

  std::optional<StreamOutputPlan> OutputFormatMap::probe(const ProbeOutputContext &ctx)
  {
    if(ctx.mVerbose) {
      SPDLOG_INFO("Probing for '{}' with extension '{}' and protocol '{}' at {} ", ctx.mFilename, ctx.mExtension, ctx.mProtocol, static_cast<void *>(this));
    }
    std::shared_lock lock(mMutex);

    auto tryFormats = [&ctx](const auto &formats) -> std::optional<StreamOutputPlan> {
      for(const auto &format : formats) {
        if(ctx.mVerbose) {
          SPDLOG_INFO("Probing format: '{}' for '{}'", format->name(), ctx.mFilename);
        }
        if(!format->supportsProtocol(ctx.mProtocol))
          continue;
        auto plan = format->probe(ctx);
        if(plan) {
          if(ctx.mVerbose) {
            SPDLOG_INFO("Found plan for '{}'", ctx.mFilename);
          }
          return plan;
        }
      }
      return std::nullopt;
    };

    auto extensionIt = mFormatByExtension.find(ctx.mExtension);
    if(extensionIt != mFormatByExtension.end()) {
      if(ctx.mVerbose) {
        SPDLOG_INFO("Found {} possible formats for '{}'", extensionIt->second.size(), ctx.mFilename);
      }
      if(auto plan = tryFormats(extensionIt->second))
        return plan;
    }

    if(!ctx.mExtension.empty()) {
      auto defaultIt = mFormatByExtension.find("");
      if(defaultIt != mFormatByExtension.end()) {
        if(auto plan = tryFormats(defaultIt->second))
          return plan;
      }
    }

    return std::nullopt;
  }

  OutputFormatMap &outputFormatMap()
  {
    static OutputFormatMap map;
    return map;
  }

}// namespace Ravl2