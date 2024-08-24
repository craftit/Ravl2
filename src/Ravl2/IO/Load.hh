
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
  //! @brief Abstract container that represents a file, that could have one or more objects.
  //! @details This class is used to represent a file that could have one or more objects.

  class StreamInputBase
  {
  public:
    virtual ~StreamInputBase() = default;

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] virtual const std::type_info &type() const = 0;

    //! @brief Read the object at the current position in the stream as a std::any
    //! @param pos - The position in the stream
    //! @return The object as a std::any.
    virtual std::any anyRead(std::streampos pos) = 0;

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
    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] const std::type_info &type() const override
    {
      return typeid(ObjectT);
    }

    //! Read the object at the current position in the stream.e
    //! @param obj - The object to read into.
    //! @param pos - The position in the stream
    //! @return True if an object was read.
    virtual std::optional<ObjectT> read(std::streampos pos) = 0;

    //! Goto next position in the stream and read the object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    virtual std::optional<ObjectT> next(std::streampos &pos) = 0;

    //! @brief Read the object at the current position in the stream as a std::any
    //! @param pos - The position in the stream
    //! @return The object as a std::any.
    std::any anyRead(std::streampos pos) final
    {
      auto obj = read(pos);
      if(obj.has_value())
        return std::move(obj.value());
      return {};
    }

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


  //! @brief Open a base stream for reading.
  //! @param url - The filename to open.
  //! @param type - The type of the object to read.
  //! @param formatHint - A hint to the format of the file.
  //! @return A pointer to the stream.

  [[nodiscard]] std::unique_ptr<StreamInputBase> openInput(const std::string &url, const std::type_info &type,  const nlohmann::json &formatHint);

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
    if(!container)
      return false;
    auto *input = dynamic_cast<StreamInputContainer<ObjectT> *>(container.get());
    if(!input) {
      SPDLOG_ERROR("Failed to cast container to InputContainer<ObjectT>");
      return false;
    }
    auto tmp = input->read(input->beginOffset());
    if(tmp.has_value()) {
      object = std::move(tmp.value());
      return true;
    }
    return false;
  }


}// namespace Ravl2