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


//! Interface class for metadata frames
template<typename DataT>
class MetaDataFrame : public MetaDataFrameBase {
public:
  //! Virtual destructor
  virtual ~MetaDataFrame() = default;

  //! Constructor with data, format, ID, and timestamp
  MetaDataFrame(const DataT& data, StreamItemId id, MediaTime timestamp)
    : MetaDataFrameBase(id, timestamp)
    , mData(data)
  {}

  //! Get the metadata as a specific type
  const DataT &data() const
  { return mData; }

  //! Get the data type name
  virtual std::string dataTypeName() const
  { return typeid(DataT).name(); }

  //! Check if the frame has valid data
  bool isValid() const override
  { return true; }

  //! Access data
  [[nodiscard]] std::any frameData() const override
  { return mData; }

protected:
  //! Constructor with ID and timestamp
  MetaDataFrame(StreamItemId id, MediaTime timestamp)
    : MetaDataFrameBase(id, timestamp)
  {}

  //! Default constructor
  MetaDataFrame() = default;

  DataT mData;             //!< The metadata
};


} // namespace Ravl2::Video
