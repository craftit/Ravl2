
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
    //! Default constructor.
    StreamOutputBase() = default;

    //! Construct a stream with a start and end offset.
    explicit StreamOutputBase(std::streampos start, std::streampos end = std::numeric_limits<std::streampos>::max())
      : mStart(start),
        mEnd(end)
    {}

    //! Make default constructor.
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
    OutputFormat(std::string format, std::string extension,int priority = 0)
      : m_format(std::move(format)),
        m_extension(std::move(extension)),
        m_priority(priority)
    {}

    //! Virtual destructor.
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

    //! Get the priority of the format.
    [[nodiscard]] int priority() const noexcept
    {
      return m_priority;
    }

    //! Plan for getting the output
    using OutputPlanT = std::tuple<std::unique_ptr<StreamOutputBase>,ConversionChain >;

    //! Test if we can save this type.
    [[nodiscard]] virtual std::optional<OutputPlanT> probe(const ProbeOutputContext &ctx) const = 0;

  private:
    std::string m_format;
    std::string m_extension; //!< Extension of the file, maybe a comma separated list.
    int m_priority = 0;
  };

  //! @brief Construct a format handler from a callback.

  class OutputFormatCall
    : public OutputFormat
  {
  public:
    //! @brief Construct a format handler from a callback.
    //! @param format - The format of the file.
    //! @param extension - The extension of the file. - If empty, the format handles multiple extensions, maybe a comma separated list.
    //! @param callback - The callback to write the object.
    OutputFormatCall(std::string format, std::string extension, int priority, std::function<std::optional<OutputFormat::OutputPlanT>(const ProbeOutputContext &)> callback)
      : OutputFormat(std::move(format), std::move(extension), priority),
        m_callback(std::move(callback))
    {}

    //! Test if we can save this type.
    [[nodiscard]] std::optional<OutputPlanT> probe(const ProbeOutputContext &ctx) const override
    {
      return m_callback(ctx);
    }

  private:
    std::function<std::optional<OutputFormat::OutputPlanT>(const ProbeOutputContext &)> m_callback;
  };

  //! @brief Map of save formats.
  //! This class is used to map file extensions to save formats.

  class OutputFormatMap
  {
  public:
    //! Add a format to the map.
    //! @param format - The format to add.
    //! @return True if the format was added.
    bool add(std::shared_ptr<OutputFormat> format);

    //! Get the format for a given extension.
    [[nodiscard]] std::optional<OutputFormat::OutputPlanT> probe(const ProbeOutputContext &ctx);

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::vector<std::shared_ptr<OutputFormat> > > m_formatByExtension;
  };

  OutputFormatMap &outputFormatMap();

  //! @brief Container that represents a file or data stream that could have one or more object to be written.

  template <typename ObjectT>
  class StreamOutputContainer
      : public StreamOutputBase
  {
  public:
    //! Default constructor.
    StreamOutputContainer() = default;

    //! Construct a stream with a start and end offset.
    explicit StreamOutputContainer(std::streampos start, std::streampos end = std::numeric_limits<std::streampos>::max())
        : StreamOutputBase(start, end)
    {}

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] const std::type_info &type() const override
    {
      return typeid(ObjectT);
    }

    //! Write an object to the stream.
    //! @param obj - The object to write.
    //! @param pos - The position in the stream where the object was written.
    //! @return True if the object was written.
    virtual std::streampos write(const ObjectT &obj, std::streampos pos) = 0;

    //! @brief Write an object to the stream.
    //! @param value - The object to write.
    //! @param pos - The position in the stream
    //! @return The position in the stream after writing the object.
    std::streampos anyWrite(std::any value,std::streampos pos) final {
      if(value.type() != typeid(ObjectT))
        throw std::bad_any_cast();
      auto obj = std::any_cast<ObjectT>(value);
      return write(obj, pos);
    }

    //! Add object to the end of the stream.
    void push_back(const ObjectT &obj)
    {
      std::streampos pos = mEnd;
      write(obj, pos);
    }

    [[nodiscard]] auto begin() const
    {
      return OutputStreamIterator<ObjectT>(*this, mStart);
    }

    [[nodiscard]] auto end() const
    {
      return OutputStreamIterator<ObjectT>(*this, mEnd);
    }
  };

  //! @brief Writer implemented with a lambda.

  template <typename ObjectT>
  class StreamOutputCall
      : public StreamOutputContainer<ObjectT>
  {
  public:
    //! @brief Construct a stream with a lambda.
    //! @param write - The lambda to write the object.
    //! @param start - The start position in the stream.
    //! @param end - The end position in the stream.
    explicit StreamOutputCall(std::function<std::streampos(const ObjectT &, std::streampos)> writeCall, std::streampos start = 0, std::streampos end = std::numeric_limits<std::streampos>::max())
        : StreamOutputContainer<ObjectT>(start, end),
          mWrite(writeCall)
    {}

    //! Write an object to the stream.
    //! @param obj - The object to write.
    //! @param pos - The position in the stream where the object was written.
    //! @return True if the object was written.
    std::streampos write(const ObjectT &obj, std::streampos pos) override
    {
      return mWrite(obj, pos);
    }
  private:
    std::function<std::streampos(const ObjectT &, std::streampos)> mWrite;
  };


}// namespace Ravl2
