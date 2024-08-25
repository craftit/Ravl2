//
// Created by charles on 24/08/24.
//

#pragma once

#include <typeinfo>
#include <nlohmann/json.hpp>
#include <utility>
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{
  //! @brief Abstract container that represents a file, that could have one or more objects.
  //! @details This class is used to represent a file that could have one or more objects.

  class StreamInputBase
  {
  public:
    //! Default constructor.
    StreamInputBase() = default;

    //! Construct a stream with a start and end offset.
    explicit StreamInputBase(std::streampos start, std::streampos end = std::numeric_limits<std::streampos>::max())
      : mStart(start),
        mEnd(end)
    {}

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
    ProbeInputContext(std::string url, std::string filename, std::string protocol, std::string ext, nlohmann::json formatHint,const std::type_info &targetType)
      : m_url(std::move(url)),
        m_filename(std::move(filename)),
        m_protocol(std::move(protocol)),
        m_extension(std::move(ext)),
        m_formatHint(std::move(formatHint)),
        m_targetType(targetType)
    {}

    std::shared_ptr<std::istream> mStream;
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

    InputFormat(std::string name, std::string extension,int priority = 0)
        : m_name(std::move(name)),
          m_extension(std::move(extension)),
          m_priority(priority)
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

    //! Get the priority of the format.
    [[nodiscard]] int priority() const noexcept
    {
      return m_priority;
    }
  private:
    std::string m_name;
    std::string m_extension;
    int m_priority = 0;
  };

  //! Wrapper for a function or lambda that can probe a file format.
  class InputFormatCall
    : public InputFormat
  {
  public:
    //! Constructor
    //! @param name - The name of the format.
    //! @param extension - Expected extension of the format.
    //! @param priority - Priority of the format. Higher is better, default is 0.
    InputFormatCall(std::string name, std::string extension, int priority, std::function<std::optional<InputPlanT>(const ProbeInputContext &)> probe)
      : InputFormat(std::move(name), std::move(extension), priority),
        m_probe(std::move(probe))
    {}

    //! @brief Test if we can load this type.
    //! @param ctx - The context to see if we can load.
    //! @return optional, if format is unknown, return nullopt, otherwise return data the format can use to load the file.
    [[nodiscard]] std::optional<InputPlanT> probe(const ProbeInputContext &ctx) const final {
      return m_probe(ctx);
    }

  private:
    std::function<std::optional<InputPlanT>(const ProbeInputContext &)> m_probe;
  };

  //! @brief Map of save formats.
  //! This class is used to map file extensions to save formats.

  class InputFormatMap
  {
  public:
    //! Add a format to the map.
    //! @param format - The format to add.
    //! @return True if the format was added.
    bool add(std::shared_ptr<InputFormat> format);

    //! Get the format for a given extension.
    [[nodiscard]] std::optional<InputFormat::InputPlanT> probe(const ProbeInputContext &ctx);

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::vector<std::shared_ptr<InputFormat> > > m_formatByExtension;
  };

  InputFormatMap &InputFormatMap();


  template <typename ObjectT>
  class InputStreamIterator;

  template <typename ObjectT>
  class OutputStreamIterator;

  //! @brief Container that represents a file or data stream that could have one or more object to be read.
  //! It allows a stream of objects to be treated as a normal c++ container, with begin and end iterators.
  //! The streampos, is implementation defined, but is used to represent the position in the stream
  //! but the exact meaning of the position is implementation defined. It could be a byte offset, or a line number,
  //! etc.

  template <typename ObjectT>
  class StreamInputContainer : public StreamInputBase
  {
  public:
    //! Default constructor.
    StreamInputContainer() = default;

    //! Construct a stream with a start and end offset.
    explicit StreamInputContainer(std::streampos start, std::streampos end = std::numeric_limits<std::streampos>::max())
        : StreamInputBase(start, end)
    {}

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] const std::type_info &type() const override
    {
      return typeid(ObjectT);
    }

    //! Goto next position in the stream and read the object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    virtual std::optional<ObjectT> next(std::streampos &pos) = 0;

    //! Goto next position in the stream and read the object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    std::any anyNext(std::streampos &pos) final
    {
      auto obj = next(pos);
      if(obj.has_value())
        return std::move(obj.value());
      return {};
    }

    //! Get start iterator.
    [[nodiscard]] auto begin() const
    {
      return InputStreamIterator<ObjectT>(*this, mStart);
    }

    //! Get end iterator.
    [[nodiscard]] auto end() const
    {
      return InputStreamIterator<ObjectT>(*this, mEnd);
    }

  protected:
  };

  //! @brief Implementation of a stream input container that reads objects from a stream via a callback.

  template <typename ObjectT>
  class StreamInputCall
      : public StreamInputContainer<ObjectT>
  {
  public:
    //! @brief Constructor.
    //! @param begin - The start of the stream.
    //! @param end - The end of the stream.
    //! @param callback - The callback to read the object.
    StreamInputCall(std::function<std::optional<ObjectT>(std::streampos &)> callback,std::streampos begin = 0, std::streampos end = std::numeric_limits<std::streampos>::max())
        : StreamInputContainer<ObjectT>(begin, end),
          mCallback(std::move(callback))
    {}

    //! Goto next position in the stream and read the object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    std::optional<ObjectT> next(std::streampos &pos) override
    {
      if(pos >= this->mEnd)
        return {};
      return mCallback(pos);
    }

  private:
    std::function<std::optional<ObjectT>(std::streampos &)> mCallback;
  };


}