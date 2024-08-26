//
// Created by charles on 25/08/24.
//

#include "Ravl2/IO/Save.hh"

namespace Ravl2
{

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
    OutputStreamIterator(StreamOutput<ObjectT> &container, std::streampos pos)
        : mContainer(&container),
          mPos(pos)
    {}

    //! @brief Construct an iterator.
    //! @details This constructor is used to construct an iterator.
    OutputStreamIterator() = default;

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
    StreamOutput<ObjectT> *mContainer = nullptr;
    std::streampos mPos;
  };

  //! @brief Output stream proxy
  //! @details This class is used to represent an output stream proxy.
  template <typename ObjectT>
  class StreamOutputProxy
  {
  public:
    StreamOutputProxy() = default;

    //! @brief Construct a stream proxy.
    //! @details This constructor is used to construct a stream proxy.
    //! @param stream - The stream to proxy.
    explicit StreamOutputProxy(std::shared_ptr<StreamOutput<ObjectT>> &&stream)
        : mStream(std::move(stream))
    {}

    //! @brief Construct a stream proxy.
    //! @details This constructor is used to construct a stream proxy.
    //! @param stream - The stream to proxy.
    explicit StreamOutputProxy(const std::shared_ptr<StreamOutput<ObjectT>> &stream)
        : mStream(stream)
    {}

    //! @brief Get the begin iterator.
    //! @return The begin iterator.
    [[nodiscard]] OutputStreamIterator<ObjectT> begin()
    {
      return OutputStreamIterator<ObjectT>(*mStream, mStream->beginOffset());
    }

    //! @brief Get the end iterator.
    //! @return The end iterator.
    [[nodiscard]] OutputStreamIterator<ObjectT> end()
    {
      return OutputStreamIterator<ObjectT>(*mStream, mStream->endOffset());
    }

    //! @brief Short cut to write an object to the end of the stream.
    //! @param object - The object to write.
    void push_back(const ObjectT &object)
    {
      assert(mStream);
      mStream->write(object, mStream->endOffset());
    }

    //! @brief Write an object to the stream.
    //! @param object - The object to write.
    void put(const ObjectT &object)
    {
      assert(mStream);
      mStream->write(object, mStream->endOffset());
    }

    //! @brief test if stream is valid, and ready to write to
    //! @return True if the stream is valid.
    [[nodiscard]] bool valid() const
    {
      return mStream != nullptr;
    }

  private:
    std::shared_ptr<StreamOutput<ObjectT>> mStream;
  };

  //! @brief Open an output stream for writing.
  //! @param url - The filename to open.
  //! @param formatHint - A hint to the format of the file.
  //! @return A handle to the stream.
  template <typename ObjectT>
  [[nodiscard]] StreamOutputProxy<ObjectT> openOutputStream(const std::string &url, const nlohmann::json &formatHint = defaultSaveFormatHint())
  {
    auto outStreamPlan = openOutput(url, typeid(ObjectT), formatHint);
    if(!outStreamPlan.has_value()) {
      SPDLOG_ERROR("Failed to open output stream for '{}'", url);
      return {};
    }
    if(outStreamPlan.value().mConversion) {
      auto outStrm = std::make_shared<StreamOutputCall<ObjectT>>([plan = outStreamPlan.value()](const ObjectT &object, std::streampos pos) -> std::streampos {
        (void)pos;
        return plan.mStream->anyWrite(plan.mConversion(std::any(object)), pos);
      },
                                                                 outStreamPlan->mStream->beginOffset(), outStreamPlan->mStream->endOffset());

      return StreamOutputProxy<ObjectT>(dynamic_pointer_cast<StreamOutput<ObjectT>>(outStrm));
    }
    auto outStream = dynamic_pointer_cast<StreamOutput<ObjectT>>(outStreamPlan.value().mStream);
    return StreamOutputProxy<ObjectT>(std::move(outStream));
  }

}// namespace Ravl2