//
// Created by charles on 24/08/24.
//

#pragma once

#include <typeinfo>
#include <nlohmann/json.hpp>
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{
  //! @brief Abstract container that represents a file, that could have one or more objects.
  //! @details This class is used to represent a file that could have one or more objects.

  class StreamInputBase
  {
  public:
    virtual ~StreamInputBase() = default;

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] virtual const std::type_info &type() const = 0;

    //! Goto next position in the stream and read the object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    virtual std::any anyNext(std::streampos &pos) = 0;

    //! Get the start offset of the stream.
    [[nodiscard]] std::streampos beginOffset() const
    {
      return mStart;
    }

    //! Get the end offset of the stream.
    [[nodiscard]] std::streampos endOffset() const
    {
      return mEnd;
    }

    //! Test if the stream is empty.
    [[nodiscard]] bool empty() const
    {
      return mStart == mEnd;
    }

  protected:
    std::streampos mStart = 0;
    std::streampos mEnd = std::numeric_limits<std::streampos>::max();
  };

  //! Information about a file we're loading.
  //! This includes the file name, and the type of data we're loading as well as the first few bytes of the file.

  class ProbeInputContext
  {
  public:
    ProbeInputContext(std::string url, const std::string &filename, std::string protocol, std::string ext, nlohmann::json formatHint,const std::type_info &targetType)
      : m_url(std::move(url)),
        m_filename(filename),
        m_protocol(std::move(protocol)),
        m_extension(std::move(ext)),
        m_formatHint(std::move(formatHint)),
        m_targetType(targetType)
    {}

    std::unique_ptr<std::istream> mStream;
    std::string m_url;
    std::string m_filename;
    std::string m_protocol;
    std::string m_extension;
    nlohmann::json m_formatHint;
    const std::type_info &m_targetType;
    std::vector<uint8_t> m_data;
  };

  //! Load file format

  class InputFormat
  {
  public:
    InputFormat() = default;

    InputFormat(std::string name, std::string extension)
        : m_name(std::move(name)),
          m_extension(std::move(extension))
    {}

    //! Make destructor virtual
    virtual ~InputFormat() = default;

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

    using InputPlanT = std::tuple<std::shared_ptr<StreamInputBase>, ConversionChain>;

    //! @brief Test if we can load this type.
    //! @param ctx - The context to see if we can load.
    //! @return optional, if format is unknown, return nullopt, otherwise return data the format can use to load the file.
    [[nodiscard]] virtual std::optional<InputPlanT> probe(const ProbeInputContext &ctx) const = 0;

  private:
    std::string m_name;
    std::string m_extension;
  };

  //! @brief Map of save formats.
  //! This class is used to map file extensions to save formats.

  class InputFormatMap
  {
  public:
    //! Add a format to the map.
    //! @param format - The format to add.
    //! @return True if the format was added.
    bool add(std::unique_ptr<InputFormat> format);

    //! Get the format for a given extension.
    [[nodiscard]] std::optional<InputFormat::InputPlanT> probe(const ProbeInputContext &ctx);

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::vector<std::unique_ptr<InputFormat> > > m_formatByExtension;
  };

  InputFormatMap &InputFormatMap();

}