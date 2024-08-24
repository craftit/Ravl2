//
// Created by charles on 24/08/24.
//

#pragma once

#include <typeinfo>
#include <nlohmann/json.hpp>
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{
  //! Information about a file we're loading.
  //! This includes the file name, and the type of data we're loading as well as the first few bytes of the file.

  class ProbeLoadContext
  {
  public:
    ProbeLoadContext(const std::string &filename, const std::type_index &targetType, const std::vector<uint8_t> &data)
        : m_filename(filename),
          m_targetType(targetType),
          m_data(data)
    {
      m_extension = filename.substr(filename.find_last_of('.') + 1);
    }

    std::unique_ptr<std::istream> mStream;
    std::string m_extension;
    std::string m_filename;
    std::type_index m_targetType;
    std::vector<uint8_t> m_data;
  };

  //! Load file format

  class LoadFormat
  {
  public:
    LoadFormat() = default;

    LoadFormat(std::string name, std::string extension)
        : m_name(std::move(name)),
          m_extension(std::move(extension))
    {}

    //! Make destructor virtual
    virtual ~LoadFormat() = default;

    //! Get the name of the format.
    [[nodiscard]] const std::string &name() const noexcept
    {
      return m_name;
    }

    //! Get the extension of the format.
    [[nodiscard]] const std::string &extension() const noexcept
    {
      return m_extension;
    }

    //! @brief Test if we can load this type.
    //! @param ctx - The context to see if we can load.
    //! @return optional, if format is unknown, return nullopt, otherwise return data the format can use to load the file.
    //[[nodiscard]] virtual std::optional<std::shared_ptr<StreamInputContainer<std::any>>> probe(const ProbeLoadContext &ctx) const = 0;

  private:
    std::string m_name;
    std::string m_extension;
  };
}