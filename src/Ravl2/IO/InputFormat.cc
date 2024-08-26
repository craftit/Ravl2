//
// Created by charles on 24/08/24.
//

#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/StringUtils.hh"


namespace Ravl2
{

  bool InputFormatMap::add(std::shared_ptr<InputFormat> format) {
    SPDLOG_TRACE("Adding format: '{}' Ext:'{}'  Map:{}", format->name(), format->extension(), static_cast<void*>(this));
    auto extensions = splitStrings(format->extension(),',');
    std::lock_guard lock(m_mutex);
    if(extensions.empty()) {
      m_formatByExtension[""].push_back(std::move(format));
      return true;
    }
    for(auto &ext : extensions) {
      SPDLOG_TRACE("Adding format: '{}' Ext:'{}' ", format->name(), ext);
      m_formatByExtension[ext].push_back(format);
    }
    return true;
  }

  std::optional<StreamInputPlan> InputFormatMap::probe(const ProbeInputContext &ctx)
  {
    std::shared_lock lock(m_mutex);
    if(ctx.m_verbose) {
      SPDLOG_INFO("Probing input for file: {}. Protocol:{}  Have {} entries in map '{}' ", ctx.m_filename, ctx.m_protocol, m_formatByExtension.size(), static_cast<void*>(this));
    }
    auto extIt = m_formatByExtension.find(ctx.m_extension);
    if(extIt != m_formatByExtension.end()) {
      const auto &fmtList = extIt->second;
      if(ctx.m_verbose) {
        SPDLOG_INFO("Probing for extension: {}, found {} handlers. ", ctx.m_extension, fmtList.size());
      }
      for(const auto &fmt : fmtList) {
        if(ctx.m_verbose) {
          SPDLOG_INFO("Probing format: {}. Supports Protocol:{} ({})", fmt->name(), fmt->supportsProtocol(ctx.m_protocol),fmt->protocol());
        }
        if(!fmt->supportsProtocol(ctx.m_protocol))
          continue;
        auto plan = fmt->probe(ctx);
        if(plan.has_value()) {
          if(ctx.m_verbose) {
            SPDLOG_INFO("Fond plan.");
          }
          return plan;
        }
      }
    }
    // Check for a default format
    auto defIt = m_formatByExtension.find("");
    if(defIt != m_formatByExtension.end()) {
      const auto &fmtList = m_formatByExtension.at("");
      for(const auto &fmt : fmtList) {
        if(ctx.m_verbose) {
          SPDLOG_INFO("Probing format: {}. Supports Protocol:{} ", fmt->name(), ctx.m_filename,fmt->supportsProtocol(ctx.m_protocol));
        }
        if(!fmt->supportsProtocol(ctx.m_protocol))
          continue;
        if(!ctx.m_extension.empty() && !fmt->supportsExtension(ctx.m_extension))
          continue;
        auto plan = fmt->probe(ctx);
        if(plan.has_value()) {
          if(ctx.m_verbose) {
            SPDLOG_INFO("Fond plan.");
          }
          return plan;
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

}