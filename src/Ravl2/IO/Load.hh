
#pragma once

#include <typeinfo>
#include <string>
#include <limits>
#include <cassert>
#include <spdlog/spdlog.h>
#include <optional>
#include <any>
#include <nlohmann/json.hpp>
#include "Ravl2/IO/StreamInput.hh"

namespace Ravl2
{

  //! @brief Open a base stream for reading.
  //! @param url - The filename to open.
  //! @param type - The type of the object to read.
  //! @param formatHint - A hint to the format of the file.
  //! @return A pointer to the stream.
  [[nodiscard]] std::optional<StreamInputPlan> openInput(const std::string &url, const std::type_info &type,  const nlohmann::json &formatHint);

  //! Default load format hint.
  [[nodiscard]] const nlohmann::json &defaultLoadFormatHint();

  //! @brief Load a file into an object.
  //! The file is loaded using the cereal library.
  //! @param url - The filename to load.
  //! @param object - The object to load into.
  //! @param formatHint - A hint to the format of the file.
  //! @return True if the file was loaded successfully.
  //!
  //! Format hints include
  //!  - "verbose" - If true, print verbose output.

  template <typename ObjectT>
  bool load(ObjectT &object, const std::string &url, const nlohmann::json &formatHint = defaultLoadFormatHint())
  {
    auto container = openInput(url, typeid(ObjectT), formatHint);
    if(!container.has_value())
      return false;
    std::streampos pos = container.value().mStream->beginOffset();
    if(!container.value().mConversion) {
      auto *input = dynamic_cast<StreamInput<ObjectT> *>(container.value().mStream.get());
      if(!input) {
        SPDLOG_ERROR("Failed to cast container to InputContainer<ObjectT>");
        return false;
      }
      auto tmp = input->next(pos);
      if(!tmp.has_value()) {
        SPDLOG_ERROR("Failed to read object from stream");
        return false;
      }
      object = std::move(tmp.value());
      return true;
    }
    auto loaded = container.value().mStream->anyNext(pos);
    if(!loaded.has_value()) {
      SPDLOG_ERROR("Failed to read object from stream");
      return false;
    }
    auto resultAny = container.value().mConversion(loaded);
    if(!resultAny.has_value()) {
      SPDLOG_ERROR("Failed to convert object from stream");
      return false;
    }
    object = std::move(std::any_cast<ObjectT>(resultAny));
    return true;
  }

  //! @brief Load a file into an object.
  template <typename ObjectT>
  std::optional<ObjectT> load(const std::string &url, const nlohmann::json &formatHint = defaultLoadFormatHint())
  {
    ObjectT object;
    if(load(object, url, formatHint))
      return object;
    return std::nullopt;
  }

}// namespace Ravl2