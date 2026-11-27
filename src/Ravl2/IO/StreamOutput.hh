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
#include <utility>

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

    //! Close the stream and finalize any pending output.
    //! @return True if the stream closed successfully.
    virtual bool close()
    {
      return true;
    }

    //! Get type name of the object.
    [[nodiscard]] std::string typeName() const;

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] virtual const std::type_info &type() const = 0;

    //! @brief Write an object to the stream.
    //! @param value - The object to write.
    //! @param pos - The position in the stream
    //! @return The position in the stream after writing the object.
    virtual std::streampos anyWrite(std::any value, std::streampos pos) = 0;

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
  struct StreamOutputPlan {
    std::shared_ptr<StreamOutputBase> mStream;
    std::function<std::any(std::any)> mConversion;
    float mCost = 1.0f;
  };

  //! @brief Container that represents a file or data stream that could have one or more object to be written.

  template <typename ObjectT>
  class StreamOutput : public StreamOutputBase
  {
  public:
    //! Default constructor.
    StreamOutput() = default;

    //! Construct a stream with a start and end offset.
    explicit StreamOutput(std::streampos start, std::streampos end = std::numeric_limits<std::streampos>::max())
        : StreamOutputBase(start, end)
    {}

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] const std::type_info &type() const final
    {
      return typeid(ObjectT);
    }

    //! Write an object to the stream.
    //! @param obj - The object to write.
    //! @param pos - The position in the stream where the object was written.
    //! @return The position after writing the object.
    virtual std::streampos write(const ObjectT &obj, std::streampos pos) = 0;

    //! @brief Write an object to the stream.
    //! @param value - The object to write.
    //! @param pos - The position in the stream
    //! @return The position in the stream after writing the object.
    std::streampos anyWrite(std::any value, std::streampos pos) final
    {
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

    [[nodiscard]] auto begin()
    {
      return OutputStreamIterator<ObjectT>(*this, mStart);
    }

    [[nodiscard]] auto end()
    {
      return OutputStreamIterator<ObjectT>(*this, mEnd);
    }
  };

  //! @brief Writer implemented with a lambda.

  template <typename ObjectT>
  class StreamOutputCall : public StreamOutput<ObjectT>
  {
  public:
    ~StreamOutputCall() override
    {
      close();
    }
    //! @brief Construct a stream with a lambda.
    //! @param write - The lambda to write the object.
    //! @param start - The start position in the stream.
    //! @param end - The end position in the stream.
    explicit StreamOutputCall(std::function<std::streampos(const ObjectT &, std::streampos)> writeCall,
                              std::streampos start = 0,
                              std::streampos end = std::numeric_limits<std::streampos>::max(),
                              std::function<bool()> closeCall = {})
        : StreamOutput<ObjectT>(start, end),
          mWrite(std::move(writeCall)),
          mClose(std::move(closeCall))
    {}

    //! Write an object to the stream.
    //! @param obj - The object to write.
    //! @param pos - The position in the stream where the object was written.
    //! @return The position after writing the object.
    std::streampos write(const ObjectT &obj, std::streampos pos) override
    {
      return mWrite(obj, pos);
    }

    //! Close the stream through its optional close callback.
    bool close() override
    {
      if(mClosed)
        return true;
      mClosed = true;
      return !mClose || mClose();
    }

  private:
    std::function<std::streampos(const ObjectT &, std::streampos)> mWrite;
    std::function<bool()> mClose;
    bool mClosed = false;
  };

}// namespace Ravl2