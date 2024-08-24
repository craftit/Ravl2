//
// Created by charles on 24/08/24.
//

#include "InputFormat.hh"

namespace Ravl2
{

  bool InputFormatMap::add(std::unique_ptr<InputFormat> format) {
    std::lock_guard lock(m_mutex);
    m_formatByExtension[format->extension()].push_back(std::move(format));
    return true;
  }

  std::optional<InputFormat::InputPlanT> InputFormatMap::probe(const ProbeInputContext &ctx)
  {
    std::shared_lock lock(m_mutex);
    (void) ctx;
    return std::nullopt;
  }
}