
#pragma once

#include <typeinfo>
#include <nlohmann/json.hpp>
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{
  //! Save file format
  //! We select the format to use on save based on the file extension, and the data type.
  //! The user may also provide json to specify the format.

  class SaveFormat
  {
  public:

      //! Default constructor.
      SaveFormat() = default;

      //! Constructor.
      SaveFormat(const std::string &format, const std::string &extension)
              : m_format(format),
                m_extension(extension)
      {}

      virtual ~SaveFormat() = default;

      //! Get the format.
      [[nodiscard]] const std::string &format() const noexcept
      {
        return m_format;
      }

      //! Get the extension.
      [[nodiscard]] const std::string &extension() const noexcept
      {
        return m_extension;
      }

      //! Test if we can save this type.
      [[nodiscard]] virtual bool probe(const std::string &extension, const std::type_info &type) const = 0;

      //! Save via std::any.
      virtual bool save(const std::any &from, const std::string &filename) const = 0;

    private:
        std::string m_format;
        std::string m_extension;
  };

  //! Information about a file we're loading.
  //! This includes the file name, and the type of data we're loading as well as the first few bytes of the file.

  class ProbeLoadContext
  {
  public:


  private:
    std::string m_extension;
    std::string m_filename;
    std::type_index m_targetType;
    std::vector<uint8_t> m_data;
  };

  //! Load file format

  class LoadFormat
  {

  };



}// namespace Ravl2
