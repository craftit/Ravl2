//
// Created by charles galambos on 20/08/2024.
//

#pragma once

#include "Ravl2/IO/Load.hh"

namespace Ravl2
{
  //! @brief c++ style Iterator object for an abstract container.
  //! @details This class is used to represent an iterator for an abstract container.
  template <typename ObjectT>
  class InputStreamIterator
  {
  public:
    using value_type = ObjectT;
    using difference_type = std::ptrdiff_t;

    //! @brief Construct an iterator.
    //! @details This constructor is used to construct an iterator.
    //! @param container - The container to iterate over.
    InputStreamIterator(const StreamInput<ObjectT> &container, std::streampos pos)
        : mContainer(&container),
          mPos(pos)
    {
      if(mContainer != nullptr && mPos < mContainer->endOffset())
        mObject = mContainer->read(mPos);
    }

    //! @brief Construct an iterator.
    //! @details This constructor is used to construct an iterator.
    InputStreamIterator() = default;

    //! @brief Get the object that the iterator is pointing to.
    //! @return The object that the iterator is pointing to.
    ObjectT &operator*() const
    {
      assert(mObject.has_value());
      return mObject.value();
    }

    //! @brief Increment the iterator.
    //! @return The iterator.
    InputStreamIterator<ObjectT> &operator++()
    {
      mObject = mContainer->next(mPos);
      return *this;
    }

    //! @brief Post increment the iterator.
    //! @return The iterator.
    InputStreamIterator<ObjectT> operator++(int)
    {
      auto copy = *this;
      ++(*this);
      return copy;
    }

    //!@brief test if the iterator is valid.
    //! @return True if the iterator is valid.
    [[nodiscard]] bool valid() const
    {
      return mContainer != nullptr && mPos < mContainer->endOffset();
    }

    //! @brief Compare two iterators.
    //! @param other - The other iterator to compare to.
    //! @return True if the iterators are equal.
    [[nodiscard]] bool operator==(const InputStreamIterator<ObjectT> &other) const
    {
      assert(mContainer == other.mContainer);
      return mPos == other.mPos;
    }

  private:
    const StreamInput<ObjectT> *mContainer = nullptr;
    std::streampos mPos;
    std::optional<ObjectT> mObject;
  };

  //! @brief Input stream proxy
  template <typename ObjectT>
  class StreamInputProxy
  {
  public:
    //! @brief Construct a default stream proxy, this is invalid.
    StreamInputProxy() = default;

    //! @brief Construct a handle to an input stream.
    //! @param stream - The stream to handle.
    explicit StreamInputProxy(const std::shared_ptr<StreamInput<ObjectT>> &stream)
        : mStream(stream)
    {}

    //! @brief Construct a handle to an input stream.
    //! @param stream - The stream to handle.
    explicit StreamInputProxy(std::shared_ptr<StreamInput<ObjectT>> &&stream)
        : mStream(std::move(stream))
    {}

    //! @brief Get the begin iterator.
    //! @return The begin iterator.
    [[nodiscard]] InputStreamIterator<ObjectT> begin() const
    {
      if(!mStream) {
        return InputStreamIterator<ObjectT>();
      }
      return InputStreamIterator<ObjectT>(*mStream, mStream->beginOffset());
    }

    //! @brief Get the end iterator.
    //! @return The end iterator.
    [[nodiscard]] InputStreamIterator<ObjectT> end() const
    {
      if(!mStream) {
        return InputStreamIterator<ObjectT>();
      }
      return InputStreamIterator<ObjectT>(*mStream, mStream->endOffset());
    }

    //! @brief Get an object from the stream.
    ObjectT get() const
    {
      if(!mStream) {
        throw std::runtime_error("Stream is not valid");
      }
      std::streampos at = mStream->endOffset();
      auto value = mStream->next(at);
      if(!value.has_value()) {
        SPDLOG_WARN("Failed to read object from stream.");
        throw std::runtime_error("Failed to read object from stream");
      }
      return value.value();
    }

    //! @brief Test if the stream is valid.
    //! @return True if the stream is valid.
    [[nodiscard]] bool valid() const
    {
      return mStream != nullptr;
    }

  private:
    std::shared_ptr<StreamInput<ObjectT>> mStream;
  };

  //! @brief Open an input stream for reading.
  //! @param url - The filename to open.
  //! @param formatHint - A hint to the format of the file.
  //! @return A handle to the stream.

  template <typename ObjectT>
  [[nodiscard]] StreamInputProxy<ObjectT> openInputStream(const std::string &url, const nlohmann::json &formatHint = defaultLoadFormatHint())
  {
    auto inStreamPlan = openInput(url, typeid(ObjectT), formatHint);
    if(!inStreamPlan.has_value()) {
      SPDLOG_ERROR("Failed to open input stream for '{}'", url);
      return {};
    }
    if(inStreamPlan.value().mConversion) {
      auto strmPtr = std::make_shared<StreamInputCall<ObjectT>>([plan = inStreamPlan.value()](std::streampos pos) -> ObjectT {
        return std::any_cast<ObjectT>(plan.mConversion(plan.mStream->anyNext(pos)));
      },
        inStreamPlan->mStream->beginOffset(), inStreamPlan->mStream->endOffset());

      return StreamInputProxy<ObjectT>(std::dynamic_pointer_cast<StreamInput<ObjectT>>(strmPtr));
    }
    auto strmPtr = std::dynamic_pointer_cast<StreamInput<ObjectT>>(inStreamPlan->mStream);
    if(!strmPtr) {
      SPDLOG_WARN("Failed to cast stream to input stream");
      throw std::runtime_error("Failed to cast stream to input stream");
    }
    return StreamInputProxy<ObjectT>(strmPtr);
  }

}// namespace Ravl2