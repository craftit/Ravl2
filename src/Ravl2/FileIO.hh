
#pragma once

#include <typeinfo>
#include <string>
#include <limits>
#include <cassert>
#include <spdlog/spdlog.h>
#include <optional>
#include "Ravl2/Sentinel.hh"

namespace Ravl2
{
  //! @brief Abstract container that represents a file, that could have one or more objects.
  //! @details This class is used to represent a file that could have one or more objects.

  class StreamBase
  {
  public:
    virtual ~StreamBase() = default;

    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] virtual const std::type_info &type() const = 0;

  };

  template <typename ObjectT>
  class InputStreamIterator;

  template <typename ObjectT>
  class OutputStreamIterator;


  //! @brief Container that represents a file or data stream that could have one or more object to be read.
  //! It allows a stream of objects to be treated as a normal c++ container, with begin and end iterators.

  template <typename ObjectT>
  class StreamInputContainer : public StreamBase
  {
  public:
    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] const std::type_info &type() const override
    { return typeid(ObjectT); }

    //! Read the object at the current position in the stream.e
    //! @param obj - The object to read into.
    //! @param pos - The position in the stream
    //! @return True if an object was read.
    virtual std::optional<ObjectT> read(std::streampos pos) = 0;

    //! Goto next position in the stream without writing an object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    virtual std::optional<ObjectT> next(std::streampos &pos) = 0;

    //! Get the start offset of the stream.
    [[nodiscard]] std::streampos beginOffset() const
    { return mStart; }

    //! Get the end offset of the stream.
    [[nodiscard]] std::streampos endOffset() const
    { return mEnd; }

    //! Test if the stream is empty.
    [[nodiscard]] bool empty() const
    { return mStart == mEnd; }

    [[nodiscard]] auto begin() const
    {  return InputStreamIterator<ObjectT>(*this,mStart); }

    [[nodiscard]] auto end() const
    {  return InputStreamIterator<ObjectT>(*this,mEnd); }
  protected:
    std::streampos mStart = 0;
    std::streampos mEnd = std::numeric_limits<std::streampos>::max();
  };

  template <typename ObjectT>
  class StreamOutputContainer : public StreamBase
  {
  public:
    //! @brief Get the type of the object.
    //! @return The type of the object.
    [[nodiscard]] const std::type_info &type() const override
    { return typeid(ObjectT); }

    //! Write an object to the stream.
    //! @param obj - The object to write.
    //! @param pos - The position in the stream where the object was written.
    //! @return True if the object was written.
    virtual bool write(const ObjectT &obj,std::streampos &pos) = 0;

    //! Goto next position in the stream without writing an object.
    //! @param pos - The position in the stream where the object was written.
    //! @return True if the next position exists.
    virtual bool next(std::streampos &pos) = 0;

    //! Add object to the end of the stream.
    void push_back(const ObjectT &obj) {
      std::streampos pos = mEnd;
      write(obj,pos);
    }

    //! Get the start offset of the stream.
    [[nodiscard]] std::streampos beginOffset() const
    { return mStart; }

    //! Get the end offset of the stream.
    [[nodiscard]] std::streampos endOffset() const
    { return mEnd; }

    [[nodiscard]] auto begin() const
    {  return OutputStreamIterator<ObjectT>(*this,mStart); }

    [[nodiscard]] auto end() const
    {  return OutputStreamIterator<ObjectT>(*this,mEnd); }

  protected:
    std::streampos mStart = 0;
    std::streampos mEnd = std::numeric_limits<std::streampos>::max();
  };


  //! @brief Open a base stream for reading.
  //! @param url - The filename to open.
  //! @param type - The type of the object to read.
  //! @param formatHint - A hint to the format of the file.
  //! @return A pointer to the stream.

  [[nodiscard]] std::unique_ptr<StreamBase> openInput(const std::string &url, const std::type_info &type,std::string_view formatHint);

  //! @brief Open a base stream for writing.
  //! @param url - The filename to open.
  //! @param type - The type of the object to write.
  //! @param formatHint - A hint to the format of the file.
  //! @return A pointer to the stream.

  [[nodiscard]] std::unique_ptr<StreamBase> openOutput(const std::string &url, const std::type_info &type,std::string_view formatHint);

  //! @brief Load a file into an object.
  //! The file is loaded using the cereal library.
  //! @param url - The filename to load.
  //! @param object - The object to load into.
  //! @param formatHint - A hint to the format of the file.
  //! @return True if the file was loaded successfully.

  template <typename ObjectT>
  bool load(ObjectT &object, const std::string &url, std::string_view formatHint = "")
  {
    auto container = openInput(url,typeid(ObjectT),formatHint);
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

  //! @brief Save an object to a file.
  //! @praam object - The object to save.
  //! @param url - The filename to save to.
  //! @param formatHint - A hint to the format of the file.
  //! @return True if the object was saved successfully.
  template <typename ObjectT>
  bool save(const std::string &url, const ObjectT &object, std::string_view formatHint = "")
  {
    auto container = openOutput(url, typeid(ObjectT), formatHint);
    if(!container)
      return false;
    auto *output = dynamic_cast<StreamOutputContainer<ObjectT> *>(container.get());
    if(!output) {
      SPDLOG_ERROR("Failed to cast container to OutputContainer<ObjectT>");
      return false;
    }
    std::streampos pos = output->beginOffset();
    return output->write(object, pos);
  }



}// namespace Ravl2