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
#include <nlohmann/json.hpp>
#include <utility>
#include "Ravl2/IO/StreamOutput.hh"

namespace Ravl2
{

  //! @brief Open a base stream for writing.
  //! @param url - The filename to open.
  //! @param type - The type of the object to write.
  //! @param formatHint - A hint to the format of the file.
  //! @return A pointer to the stream.
  [[nodiscard]] std::optional<StreamOutputPlan> openOutput(const std::string &url, const std::type_info &type, const nlohmann::json &formatHint);

  //! Default save format hint.
  [[nodiscard]] const nlohmann::json &defaultSaveFormatHint(bool verbose = false);

  //! @brief Save an object to a file.
  //! @param object - The object to save.
  //! @param url - The filename to save to.
  //! @param formatHint - A hint to the format of the file.
  //! @return True if the object was saved successfully.
  //!
  //! Format hints include
  //!  - "verbose" - If true, print verbose output.
  template <typename ObjectT>
  bool ioSave(const std::string &url, const ObjectT &object, const nlohmann::json &formatHint = defaultSaveFormatHint())
  {
    auto thePlan = openOutput(url, typeid(ObjectT), formatHint);
    if(!thePlan.has_value())
      return false;
    if(!thePlan.value().mConversion) {
      auto *output = dynamic_cast<StreamOutput<ObjectT> *>(thePlan->mStream.get());
      // Can we write directly to the stream?
      if(output != nullptr) {
        return output->write(object, output->beginOffset());
      }
    }
    return thePlan.value().mStream->anyWrite(thePlan.value().mConversion(std::any(object)), thePlan->mStream->beginOffset()) >= 0;
  }

}// namespace Ravl2