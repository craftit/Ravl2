//
// Created by charles on 24/08/24.
//

#include <algorithm>

#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/StringUtils.hh"

namespace Ravl2
{

  bool InputFormatMap::add(std::shared_ptr<InputFormat> format)
  {
    SPDLOG_TRACE("Adding format: '{}' Ext:'{}'  Map:{}", format->name(), format->extension(), static_cast<void *>(this));
    std::vector<std::string> extensions = splitStrings(format->extension(), ',');
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
    for(auto const &ext : extensions) {
      SPDLOG_TRACE("Adding format: '{}' Ext:'{}' ", format->name(), ext);
      addToExtension(ext);
    }
    return true;
  }

  std::optional<StreamInputPlan> InputFormatMap::probe(const ProbeInputContext &ctx)
  {
    std::shared_lock lock(mMutex);
    if(ctx.mVerbose) {
      SPDLOG_INFO("Probing input for file: {}. Protocol:{}  Have {} entries in map '{}' ", ctx.mFilename, ctx.mProtocol, mFormatByExtension.size(), static_cast<void *>(this));
    }
    auto extIt = mFormatByExtension.find(ctx.mExtension);
    if(extIt != mFormatByExtension.end()) {
      const auto &fmtList = extIt->second;
      if(ctx.mVerbose) {
        SPDLOG_INFO("Probing for extension: {}, found {} handlers. ", ctx.mExtension, fmtList.size());
      }
      for(const auto &fmt : fmtList) {
        if(ctx.mVerbose) {
          SPDLOG_INFO("Probing format: {}. Supports Protocol:{} ({})", fmt->name(), fmt->supportsProtocol(ctx.mProtocol), fmt->protocol());
        }
        if(!fmt->supportsProtocol(ctx.mProtocol))
          continue;
        auto plan = fmt->probe(ctx);
        if(plan.has_value()) {
          if(ctx.mVerbose) {
            SPDLOG_INFO("Found plan.");
          }
          return plan;
        }
      }
    }
    // Check for a default format
    if(!ctx.mExtension.empty()) {
      auto defIt = mFormatByExtension.find("");
      if(defIt != mFormatByExtension.end()) {
        const auto &fmtList = defIt->second;
        for(const auto &fmt : fmtList) {
          if(ctx.mVerbose) {
            SPDLOG_INFO("Probing format: {}. Supports Protocol:{} ", fmt->name(), fmt->supportsProtocol(ctx.mProtocol));
          }
          if(!fmt->supportsProtocol(ctx.mProtocol))
            continue;
          auto plan = fmt->probe(ctx);
          if(plan.has_value()) {
            if(ctx.mVerbose) {
              SPDLOG_INFO("Found plan.");
            }
            return plan;
          }
        }
      }
    }

    return std::nullopt;
  }

  InputFormatMap &inputFormatMap()
  {
    static InputFormatMap map;
    return map;
  }

}// namespace Ravl2