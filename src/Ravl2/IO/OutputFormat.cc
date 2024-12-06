//
// Created by charles on 23/08/24.
//

#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/StringUtils.hh"

namespace Ravl2
{
  bool OutputFormatMap::add(std::shared_ptr<OutputFormat> format)
  {
    auto extensions = splitStrings(format->extension(), ',');
    SPDLOG_TRACE("Adding format: '{}' Ext:'{}' ({}) Map:{}", format->name(), format->extension(),extensions.size(), static_cast<void *>(this));
    std::lock_guard lock(m_mutex);
    if(extensions.empty()) {
      m_formatByExtension[""].push_back(std::move(format));
      return true;
    }
    for(const auto &ext : extensions) {
      auto it = m_formatByExtension.find(ext);
      if(it == m_formatByExtension.end()) {
        m_formatByExtension.emplace(ext, std::vector<std::shared_ptr<OutputFormat>>{format});
      } else {
       it->second.push_back(format);
      }
    }
    return true;
  }

  std::optional<StreamOutputPlan> OutputFormatMap::probe(const ProbeOutputContext &ctx)
  {
    if(ctx.m_verbose) {
      SPDLOG_INFO("Probing for '{}' with extension '{}' and protocol '{}' at {} ", ctx.m_filename, ctx.m_extension, ctx.m_protocol, static_cast<void *>(this));
    }
    std::shared_lock lock(m_mutex);

    auto it = m_formatByExtension.find(ctx.m_extension);
    if(it == m_formatByExtension.end())
      return std::nullopt;
    if(ctx.m_verbose) {
      SPDLOG_INFO("Found {} possible formats for '{}'", it->second.size(), ctx.m_filename);
    }

    for(const auto &format : it->second) {
      if(ctx.m_verbose) {
        SPDLOG_INFO("Probing format: '{}' for '{}'", format->name(), ctx.m_filename);
      }
      auto plan = format->probe(ctx);
      if(plan) {
        if(ctx.m_verbose) {
          SPDLOG_INFO("Found plan for '{}'", ctx.m_filename);
        }
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