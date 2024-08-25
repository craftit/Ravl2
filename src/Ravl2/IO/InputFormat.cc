//
// Created by charles on 24/08/24.
//

#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/StringUtils.hh"

namespace Ravl2
{

  bool InputFormatMap::add(std::shared_ptr<InputFormat> format) {
    auto extensions = splitStrings(format->extension(),',');
    std::lock_guard lock(m_mutex);
    if(extensions.empty()) {
      m_formatByExtension[""].push_back(std::move(format));
      return true;
    }
    for(auto &ext : extensions) {
      m_formatByExtension[ext].push_back(std::move(format));
    }
    return true;
  }

  std::optional<StreamInputPlan> InputFormatMap::probe(const ProbeInputContext &ctx)
  {
    std::shared_lock lock(m_mutex);
    (void) ctx;
    return std::nullopt;
  }

  InputFormatMap &inputFormatMap()
  {
    static InputFormatMap map;
    return map;
  }

}