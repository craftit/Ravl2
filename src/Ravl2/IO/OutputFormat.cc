//
// Created by charles on 23/08/24.
//

#include "Ravl2/IO/OutputFormat.hh"

namespace Ravl2
{
  bool SaveFormatMap::add(std::unique_ptr<OutputFormat> format)
  {
    std::lock_guard lock(m_mutex);
    m_formatByExtension[format->extension()].push_back(std::move(format));
    return true;
  }

  std::optional<OutputFormat::OutputPlanT> SaveFormatMap::probe(const ProbeOutputContext &ctx)
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

  SaveFormatMap &saveFormatMap()
  {
    static SaveFormatMap map;
    return map;
  }



}