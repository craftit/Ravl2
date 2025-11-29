//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/GoPro/GpmfFrame.hh"
#include "Ravl2/GoPro/GpmfTypes.hh"

namespace Ravl2::GoPro
{
  //! Frame containing GPS telemetry data
  //! Uses RAVL2's GPSCoordinate for full geodetic support
  class GpsFrame : public Video::MetaDataFrame<GpsFix>
  {
  public:
    //! Constructor with GPS data, ID, and timestamp
    GpsFrame(const GpsFix& data,
             Video::StreamItemId id,
             Video::MediaTime timestamp)
      : Video::MetaDataFrame<GpsFix>(data, id, timestamp)
    {}

    //! Get the data type name
    [[nodiscard]] std::string dataTypeName() const override
    {
      return "GoPro::GpsFix";
    }

    //! Convenience: Get the GPSCoordinate directly
    [[nodiscard]] const GPSCoordinate& location() const { return data().location; }
  };

} // namespace Ravl2::GoPro
