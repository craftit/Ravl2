//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/Video/MetaDataFrame.hh"
#include "Ravl2/GoPro/GpmfTypes.hh"

namespace Ravl2::GoPro
{
  //! Base class for all GPMF frames
  //! This is just a marker/documentation class
  //! Actual frames derive from Video::MetaDataFrame<T>
  class GpmfFrameBase : public Video::MetaDataFrameBase
  {
  public:
    //! Virtual destructor
    ~GpmfFrameBase() override = default;

  protected:
    //! Constructor with ID and timestamp
    GpmfFrameBase(Video::StreamItemId id, Video::MediaTime timestamp)
      : Video::MetaDataFrameBase(id, timestamp)
    {}

    //! Default constructor
    GpmfFrameBase() = default;
  };

} // namespace Ravl2::GoPro
