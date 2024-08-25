
#pragma once

#include <typeinfo>
#include <string>
#include <limits>
#include <cassert>
#include <spdlog/spdlog.h>
#include <optional>
#include <any>
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/InputFormat.hh"

namespace Ravl2
{

  //! @brief Open a base stream for reading.
  //! @param url - The filename to open.
  //! @param type - The type of the object to read.
  //! @param formatHint - A hint to the format of the file.
  //! @return A pointer to the stream.

  [[nodiscard]] std::optional<InputFormat::InputPlanT> openInput(const std::string &url, const std::type_info &type,  const nlohmann::json &formatHint);

  //! @brief Load a file into an object.
  //! The file is loaded using the cereal library.
  //! @param url - The filename to load.
  //! @param object - The object to load into.
  //! @param formatHint - A hint to the format of the file.
  //! @return True if the file was loaded successfully.

  template <typename ObjectT>
  bool load(ObjectT &object, const std::string &url, const nlohmann::json &formatHint = {})
  {
    auto container = openInput(url, typeid(ObjectT), formatHint);
    if(!container.has_value())
      return false;
    std::streampos pos = get<0>(container.value())->beginOffset();
    if(get<1>(container.value()).size() == 0) {
      auto *input = dynamic_cast<StreamInputContainer<ObjectT> *>(get<0>(container.value()).get());
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
    auto loaded = get<0>(container.value())->anyNext(pos);
    if(!loaded.has_value()) {
      SPDLOG_ERROR("Failed to read object from stream");
      return false;
    }
    auto resultAny = get<1>(container.value()).convert(loaded);
    if(!resultAny.has_value()) {
      SPDLOG_ERROR("Failed to convert object from stream");
      return false;
    }
    object = std::move(std::any_cast<ObjectT>(resultAny));
    return true;
  }


}// namespace Ravl2