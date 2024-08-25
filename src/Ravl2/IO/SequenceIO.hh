//
// Created by charles galambos on 20/08/2024.
//

#pragma once

#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"

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
    InputStreamIterator(const StreamInputContainer<ObjectT> &container, std::streampos pos)
        : mContainer(&container),
          mPos(pos)
    {
      if(mContainer != nullptr && mPos < mContainer->endOffset())
        mObject = mContainer->read(mPos);
    }

    //! @brief Construct an iterator.
    //! @details This constructor is used to construct an iterator.
    InputStreamIterator()
        : mContainer(nullptr)
    {}

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
    const StreamInputContainer<ObjectT> *mContainer;
    std::streampos mPos;
    std::optional<ObjectT> mObject;
  };

  //! @brief Output iterator for an abstract container.
  //! @details This class is used to represent an output iterator for an abstract container.
  template <typename ObjectT>
  class OutputStreamIterator
  {
  public:
    using iterator_category = std::output_iterator_tag;
    using value_type = ObjectT;
    using difference_type = void;
    using pointer = void;

    //! @brief Construct an iterator.
    //! @details This constructor is used to construct an iterator.
    //! @param container - The container to iterate over.
    OutputStreamIterator(StreamOutputContainer<ObjectT> &container, std::streampos pos)
        : mContainer(&container),
          mPos(pos)
    {}

    //! @brief Construct an iterator.
    //! @details This constructor is used to construct an iterator.
    OutputStreamIterator()
        : mContainer(nullptr)
    {}

    //! @brief Write an object to the container.
    auto &operator=(const ObjectT &object)
    {
      mContainer->write(object, mPos);
      return *this;
    }

    //! @brief Increment the iterator.
    //! @return The iterator.
    OutputStreamIterator<ObjectT> &operator++()
    {
      mContainer->next(mPos);
      return *this;
    }

    //! @brief Post increment the iterator.
    //! @return The iterator.
    OutputStreamIterator<ObjectT> operator++(int)
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
    [[nodiscard]] bool operator==(const OutputStreamIterator<ObjectT> &other) const
    {
      assert(mContainer == other.mContainer);
      return mPos == other.mPos;
    }

  private:
    StreamOutputContainer<ObjectT> *mContainer;
    std::streampos mPos;
  };


#if 0
  //! @brief Open an input stream for reading.
  //! @param url - The filename to open.
  //! @param formatHint - A hint to the format of the file.
  //! @return A handle to the stream.

  template <typename ObjectT>
  [[nodiscard]] StreamInput<ObjectT> inputStream(const std::string &url,const nlohmann::json &formatHint = defaultLoadFormatHint())
  {
    auto inStream = openInput(url, typeid(ObjectT), formatHint);

    return StreamInput<ObjectT>(static_cast<StreamInputContainer<ObjectT>>(std::move(inStream)));
  }

  //! @brief Open an output stream for writing.
  //! @param url - The filename to open.
  //! @param formatHint - A hint to the format of the file.
  //! @return A handle to the stream.
  template <typename ObjectT>
  [[nodiscard]] StreamOutput<ObjectT> outputStream(const std::string &url, const nlohmann::json &formatHint = defaultSaveFormatHint())
  {
    auto outStream = openOutput(url, typeid(ObjectT), formatHint);
    return StreamOutput<ObjectT>(static_cast<StreamOutputContainer<ObjectT>>(std::move(outStream)));
  }
#endif

}// namespace Ravl2