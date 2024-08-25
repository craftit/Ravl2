//
// Created by charles on 24/08/24.
//

#pragma once

#include <typeinfo>
#include <string>
#include <limits>
#include <cassert>
#include <spdlog/spdlog.h>
#include <optional>
#include <any>
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include <nlohmann/json.hpp>
#include <utility>

namespace Ravl2
{

  //! @brief Open a base stream for writing.
  //! @param url - The filename to open.
  //! @param type - The type of the object to write.
  //! @param formatHint - A hint to the format of the file.
  //! @return A pointer to the stream.
  [[nodiscard]] std::optional<OutputFormat::OutputPlanT> openOutput(const std::string &url, const std::type_info &type, const nlohmann::json &formatHint);

  //! @brief Save an object to a file.
  //! @praam object - The object to save.
  //! @param url - The filename to save to.
  //! @param formatHint - A hint to the format of the file.
  //! @return True if the object was saved successfully.
  template <typename ObjectT>
  bool save(const std::string &url, const ObjectT &object, const nlohmann::json &formatHint = {})
  {
    auto thePlan = openOutput(url, typeid(ObjectT),formatHint);
    if(!thePlan.has_value())
      return false;
    if(get<1>(thePlan.value()).size() == 0) {
      auto *output = dynamic_cast<StreamOutputContainer<ObjectT> *>(get<0>(thePlan.value()));
      // Can we write directly to the stream?
      if(output != nullptr) {
        return output->write(object, output->beginOffset());
      }
    }
    return get<0>(thePlan.value())->anyWrite(get<1>(thePlan.value()).convert(std::any(object)), get<0>(thePlan.value())->beginOffset());
  }


}