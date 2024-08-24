//
// Created by charles on 23/08/24.
//

#include "Ravl2/IO/OutputFormat.hh"

namespace Ravl2
{
  void SaveFormatMap::add(std::unique_ptr<SaveFormat> format)
  {
    std::lock_guard lock(m_mutex);
    m_formatByExtension[format->extension()].push_back(std::move(format));
  }

  std::optional<SaveFormat::OutputPlanT> SaveFormatMap::probe(const ProbeSaveContext &ctx)
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