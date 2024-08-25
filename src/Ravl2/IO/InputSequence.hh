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
    //! @brief Construct a handle to an input stream.
    //! @param stream - The stream to handle.
    explicit StreamInputProxy(const std::shared_ptr<StreamInput<ObjectT> > &stream)
      : mStream(&stream)
    {}

    //! @brief Get the begin iterator.
    //! @return The begin iterator.
    [[nodiscard]] InputStreamIterator<ObjectT> begin() const
    {
      return InputStreamIterator<ObjectT>(*mStream, mStream->beginOffset());
    }

    //! @brief Get the end iterator.
    //! @return The end iterator.
    [[nodiscard]] InputStreamIterator<ObjectT> end() const
    {
      return InputStreamIterator<ObjectT>(*mStream, mStream->endOffset());
    }

    //! @brief Get an object from the stream.
    ObjectT get() const
    {
      std::streampos at = mStream->endOffset();
      auto value = mStream->next(at);
      if(!value.has_value()) {
        throw std::runtime_error("Failed to read object from stream");
      }
      return value.value();
    }

  private:
    std::shared_ptr<StreamInput<ObjectT> > mStream;
  };

  //! @brief Open an input stream for reading.
  //! @param url - The filename to open.
  //! @param formatHint - A hint to the format of the file.
  //! @return A handle to the stream.

  template <typename ObjectT>
  [[nodiscard]] StreamInputProxy<ObjectT> inputStream(const std::string &url,const nlohmann::json &formatHint = defaultLoadFormatHint())
  {
    auto inStreamPlan = openInput(url, typeid(ObjectT), formatHint);
    if(!inStreamPlan.has_value()) {
      return StreamInput<ObjectT>();
    }
    if(inStreamPlan.value().mConversion) {
      return StreamInputCall<ObjectT>([plan = inStreamPlan.value()](std::streampos pos) -> ObjectT {
        return std::any_cast<ObjectT>(plan.mConversion(plan.mStream->anyNext(pos)));
      }, inStreamPlan->mStream->beginOffset(), inStreamPlan->mStream->endOffset());
    }
    auto strmPtr = std::dynamic_pointer_cast<StreamInput<ObjectT>>(inStreamPlan->mStream);
    return StreamInput<ObjectT>(std::move(strmPtr));
  }


}// namespace Ravl2