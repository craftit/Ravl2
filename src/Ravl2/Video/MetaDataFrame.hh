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

namespace Ravl2 {
namespace Video {

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
    // This will throw if the data type doesn't match
    return getDataImpl<DataT>();
  }

  //! Get the data type name
  virtual std::string dataTypeName() const = 0;

  //! Check if the frame has valid data
  virtual bool isValid() const = 0;

protected:
  //! Constructor with ID and timestamp
  MetaDataFrame(StreamItemId id, MediaTime timestamp)
    : MetaDataFrameBase(id, timestamp)
  {}

  //! Default constructor
  MetaDataFrame() = default;

  //! Implementation of getting data for a specific type
  template<typename DataT>
  virtual DataT getDataImpl() const = 0;
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
  //! Implementation of getting data for a specific type
  template<typename T>
  T getDataImpl() const override {
    if (typeid(T) != typeid(DataT)) {
      throw std::runtime_error("Data type mismatch when accessing metadata");
    }
    return m_data;
  }

private:
  DataT m_data;             //!< The metadata
  std::string m_format;     //!< Format identifier for this metadata
};

//! Convenience type for GPS coordinate data
struct GPSData {
  double latitude;
  double longitude;
  double altitude;
  double accuracy;
  MediaTime timestamp;  // Some sensors might have their own timestamps
};

//! Convenience type for accelerometer data
struct AccelerometerData {
  double x;
  double y;
  double z;
  MediaTime timestamp;
};

//! Convenience type for gyroscope data
struct GyroscopeData {
  double x;
  double y;
  double z;
  MediaTime timestamp;
};

//! Convenience type for compass/magnetometer data
struct CompassData {
  double heading;
  double accuracy;
  MediaTime timestamp;
};

// Create convenience factory methods for common metadata types
namespace MetaDataFactory {

//! Create a GPS metadata frame
inline std::shared_ptr<MetaDataFrame> createGPS(double latitude, double longitude, double altitude,
                                              double accuracy, StreamItemId id, MediaTime timestamp) {
  GPSData data{latitude, longitude, altitude, accuracy, timestamp};
  return MetaDataFrame::create<GPSData>(data, "GPS", id, timestamp);
}

//! Create an accelerometer metadata frame
inline std::shared_ptr<MetaDataFrame> createAccelerometer(double x, double y, double z,
                                                        StreamItemId id, MediaTime timestamp) {
  AccelerometerData data{x, y, z, timestamp};
  return MetaDataFrame::create<AccelerometerData>(data, "ACCELEROMETER", id, timestamp);
}

//! Create a gyroscope metadata frame
inline std::shared_ptr<MetaDataFrame> createGyroscope(double x, double y, double z,
                                                    StreamItemId id, MediaTime timestamp) {
  GyroscopeData data{x, y, z, timestamp};
  return MetaDataFrame::create<GyroscopeData>(data, "GYROSCOPE", id, timestamp);
}

//! Create a compass metadata frame
inline std::shared_ptr<MetaDataFrame> createCompass(double heading, double accuracy,
                                                  StreamItemId id, MediaTime timestamp) {
  CompassData data{heading, accuracy, timestamp};
  return MetaDataFrame::create<CompassData>(data, "COMPASS", id, timestamp);
}

} // namespace MetaDataFactory

} // namespace Video
} // namespace Ravl2
