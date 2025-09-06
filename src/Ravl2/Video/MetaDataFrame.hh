//
// Created on September 6, 2025
//

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <any>
#include <map>
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Video/Frame.hh"

namespace Ravl2::Video {

//! Base class for metadata frames, regardless of specific data type
class MetaDataFrameBase : public Frame {
public:
  //! Get the metadata format identifier (e.g., "GPS", "ACCELEROMETER", "GYRO", etc.)
  virtual std::string format() const = 0;

  //! Get the stream type
  StreamType streamType() const override {
    return StreamType::Data;
  }

protected:
  //! Constructor with ID and timestamp
  MetaDataFrameBase(StreamItemId id, MediaTime timestamp)
    : Frame(id, timestamp)
  {}

  //! Default constructor
  MetaDataFrameBase() = default;
};

//! Forward declarations for specializations
template<typename DataT>
class MetaDataFrameImpl;

//! Interface class for metadata frames
class MetaDataFrame : public MetaDataFrameBase {
public:
  //! Virtual destructor
  virtual ~MetaDataFrame() = default;

  //! Create a metadata frame with a specific data type
  template<typename DataT>
  static std::shared_ptr<MetaDataFrame> create(const DataT& data,
                                              const std::string& format,
                                              StreamItemId id,
                                              MediaTime timestamp) {
    return std::make_shared<MetaDataFrameImpl<DataT>>(data, format, id, timestamp);
  }

  //! Get the metadata as a specific type
  template<typename DataT>
  DataT getData() const {
    if (dataTypeName() != typeid(DataT).name()) {
      throw std::runtime_error("Data type mismatch when accessing metadata");
    }
    return getDataTyped(static_cast<const DataT*>(nullptr));
  }

  //! Get the data type name
  virtual std::string dataTypeName() const = 0;

  //! Check if the frame has valid data
  virtual bool isValid() const override = 0;

protected:
  //! Constructor with ID and timestamp
  MetaDataFrame(StreamItemId id, MediaTime timestamp)
    : MetaDataFrameBase(id, timestamp)
  {}

  //! Default constructor
  MetaDataFrame() = default;

  //! Type-erased template method for getting data
  template<typename DataT>
  DataT getDataTyped(const DataT*) const {
    throw std::runtime_error("Unsupported data type");
  }
};

//! Template implementation of MetaDataFrame for a specific data type
template<typename DataT>
class MetaDataFrameImpl : public MetaDataFrame {
public:
  //! Constructor with data, format, ID, and timestamp
  MetaDataFrameImpl(const DataT& data, const std::string& format, StreamItemId id, MediaTime timestamp)
    : MetaDataFrame(id, timestamp)
    , m_data(data)
    , m_format(format)
  {}

  //! Get the format
  std::string format() const override { return m_format; }

  //! Get the data type name
  std::string dataTypeName() const override {
    return typeid(DataT).name();
  }

  //! Check if the frame has valid data
  bool isValid() const override {
    return true; // Basic validation - extend as needed for specific data types
  }

protected:
  //! Specialization for the actual data type
  DataT getDataTyped(const DataT*) const {
    return m_data;
  }

private:
  DataT m_data;             //!< The metadata
  std::string m_format;     //!< Format identifier for this metadata
};


} // namespace Ravl2::Video
