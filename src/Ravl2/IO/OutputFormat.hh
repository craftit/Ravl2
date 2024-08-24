
#pragma once

#include <typeinfo>
#include <nlohmann/json.hpp>
#include <utility>
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{
  //! @brief Abstract container that represents a file, that could have one or more objects.
  //! @details This class is used to represent a file that could have one or more objects.

  class StreamOutputBase
  {
  public:
    virtual ~StreamOutputBase() = default;

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] virtual const std::type_info &type() const = 0;

    //! @brief Write an object to the stream.
    //! @param value - The object to write.
    //! @param pos - The position in the stream
    //! @return The position in the stream after writing the object.
    virtual std::streampos anyWrite(std::any value,std::streampos pos) = 0;

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

  //! Information about a file we're saving.

  class ProbeOutputContext
  {
  public:
    ProbeOutputContext(std::string url, const std::string &filename, std::string protocol, std::string ext, nlohmann::json formatHint,const std::type_info &sourceType)
        : m_url(std::move(url)),
          m_filename(filename),
          m_protocol(std::move(protocol)),
          m_extension(std::move(ext)),
          m_formatHint(std::move(formatHint)),
          m_sourceType(sourceType)
    {
      m_extension = filename.substr(filename.find_last_of('.') + 1);
    }

    std::string m_url; //! The URL of the file.
    std::string m_filename; //! The name of the file, with the extension but without the protocol.
    std::string m_protocol; //! The protocol used to save the file. http, file, etc.
    std::string m_extension;
    nlohmann::json m_formatHint;
    const std::type_info &m_sourceType;
  };


  //! Save file format
  //! We select the format to use on save based on the file extension, and the data type.
  //! The user may also provide json to specify the format.

  class OutputFormat
  {
  public:

    //! Default constructor.
    OutputFormat() = default;

    //! Constructor.
    OutputFormat(std::string format, std::string extension)
      : m_format(std::move(format)),
        m_extension(std::move(extension))
    {}

    virtual ~OutputFormat() = default;

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

    //! Plan for getting the output
    using OutputPlanT = std::tuple<std::unique_ptr<StreamOutputBase>,ConversionChain >;

    //! Test if we can save this type.
    [[nodiscard]] virtual std::optional<OutputPlanT> probe(const ProbeOutputContext &ctx) const = 0;

  private:
    std::string m_format;
    std::string m_extension;
  };


  //! @brief Map of save formats.
  //! This class is used to map file extensions to save formats.

  class SaveFormatMap
  {
  public:
    //! Add a format to the map.
    //! @param format - The format to add.
    //! @return True if the format was added.
    bool add(std::unique_ptr<OutputFormat> format);

    //! Get the format for a given extension.
    [[nodiscard]] std::optional<OutputFormat::OutputPlanT> probe(const ProbeOutputContext &ctx);

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::vector<std::unique_ptr<OutputFormat> > > m_formatByExtension;
  };

  SaveFormatMap &saveFormatMap();


}// namespace Ravl2
