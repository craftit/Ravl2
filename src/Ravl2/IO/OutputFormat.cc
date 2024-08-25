//
// Created by charles on 23/08/24.
//

#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/StringUtils.hh"

namespace Ravl2
{
  bool OutputFormatMap::add(std::shared_ptr<OutputFormat> format)
  {
    auto extensions = splitStrings(format->extension(),',');
    std::lock_guard lock(m_mutex);
    if(extensions.empty()) {
      m_formatByExtension[""].push_back(std::move(format));
      return true;
    }
    for(auto &ext : extensions)
      m_formatByExtension[ext].push_back(format);
    return true;
  }

  std::optional<OutputFormat::OutputPlanT> OutputFormatMap::probe(const ProbeOutputContext &ctx)
  {
    std::shared_lock lock(m_mutex);
    auto it = m_formatByExtension.find(ctx.m_extension);
    if(it == m_formatByExtension.end())
      return std::nullopt;

    for(const auto &format : it->second)
    {
      auto plan = format->probe(ctx);
      if(plan)
        return plan;
    }

    return std::nullopt;
  }

  OutputFormatMap &outputFormatMap()
  {
    static OutputFormatMap map;
    return map;
  }



}