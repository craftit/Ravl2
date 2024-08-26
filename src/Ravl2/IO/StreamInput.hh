//
// Created by charles on 25/08/24.
//

#pragma once

#include <typeinfo>
#include <iostream>
#include <limits>
#include <any>
#include <functional>
#include <memory>
#include <optional>

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

    //! Get type name of the object.
    [[nodiscard]] std::string typeName() const;

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

  //! Plan for getting the output
  struct StreamInputPlan
  {
    std::shared_ptr<StreamInputBase> mStream;
    std::function<std::any(std::any)> mConversion;
    float mCost = 1.0f;
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
  class StreamInput : public StreamInputBase
  {
  public:
    //! Default constructor.
    StreamInput() = default;

    //! Construct a stream with a start and end offset.
    explicit StreamInput(std::streampos start, std::streampos end = std::numeric_limits<std::streampos>::max())
        : StreamInputBase(start, end)
    {}

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] const std::type_info &type() const final
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
      : public StreamInput<ObjectT>
  {
  public:
    //! @brief Constructor.
    //! @param begin - The start of the stream.
    //! @param end - The end of the stream.
    //! @param callback - The callback to read the object.
    explicit StreamInputCall(std::function<std::optional<ObjectT>(std::streampos &)> callback,std::streampos begin = 0, std::streampos end = std::numeric_limits<std::streampos>::max())
        : StreamInput<ObjectT>(begin, end),
          mCallback(std::move(callback))
    {}

    //! Goto next position in the stream and read the object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    std::optional<ObjectT> next(std::streampos &pos) override
    {
      return mCallback(pos);
    }

  private:
    std::function<std::optional<ObjectT>(std::streampos &)> mCallback;
  };

}
