// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2010, OmniPerception Ltd
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
// Modified for Ravl2 29/11/2025 by Charles Galambos.

#pragma once

#include <string>
#include "Ravl2/Types.hh"

namespace Ravl2
{
  
  //! @brief GPS Coordinate information.
  // This class uses the GRS84 Ellipsoid.
  // The raw points are stored as:  Latitude , Longitude, Height.
  // Angles are in degree's
  
  class GPSCoordinate
  {
  public:
    using RealT = double;

    //! Default constructor.
    GPSCoordinate() = default;

    //! Construct from raw angles.
    //! Note: The default standard deviation for height and position of 10 cm and 15cm is rather
    //! generous.
    GPSCoordinate(RealT latitude,
                   RealT longitude,
                   RealT height = 0,
                   RealT verticalError = 0.1,
                   RealT horizontalError = 0.15
                   )
      : mAt({latitude,longitude,height}),
        mHorizontalError(horizontalError),
        mVerticalError(verticalError)
    {}
    
    //! Constructor.
    //! Note: The default standard deviation for height and position of 10 cm and 15cm is rather
    //! generous.
    explicit GPSCoordinate(const Point<double,3> &val,
                   RealT verticalError = 0.1,
                   RealT horizontalError = 0.15
                   )
      : mAt(val),
        mHorizontalError(horizontalError),
        mVerticalError(verticalError)
    {}
    
    //! Convert from coordinates in text form.
    //! The string must have at least 2 comma separated values for
    //! latitude and longitude followed by height. If height is omitted
    //! it is assumed to be zero.
    //! Form: latitude,longitude,[height]
    //! Longitude may be either expressed as decimal angles, or in degree's minutes seconds.
    //! @oaram text Input text string.
    explicit GPSCoordinate(const std::string &text);
    
    //! @brief Convert a gps co-ordinate from text form to a GPS class.
    //! @return false if text is not correctly formatted
    static bool text2GPS(const std::string &text,GPSCoordinate &gps);
    
    //! Convert a Cartesian position to gps.
    static GPSCoordinate cartesian2GPS(const Point<double,3> &position,RealT precision = 1e-12);

    //! Print as text in degree's minutes seconds N/S E/W
    //! @return text formated coordinates
    std::string textDMS() const;

    //! Get the raw GPS coordinates.
    Point<double,3> raw() const
    { return mAt; }

    //! Convert GPS coordinate into global Cartesian coordinates.
    Point<double,3> cartesian() const;
    
    //! Access the local vertical direction of gravity.
    Vector<double,3> vertical() const;

    //! Interpolate between two GPS positions.
    //! Fraction is between 0.0 and 1.0, where 0 is p1, and 1 is p2.
    //! This currently just does a linear interpolation of all the gps parameters.
    static bool bilinearInterpolate(RealT fraction,const GPSCoordinate &p1,const GPSCoordinate &p2,GPSCoordinate &position);

    //! Access latitude in degree's
    const RealT &latitude() const
    { return mAt[0]; }
    
    //! Access longitude in degree's
    const RealT &longitude() const
    { return mAt[1]; }
    
    //! Height in meters
    const RealT &height() const
    { return mAt[2]; }

    //! Access latitude in degree's
    RealT &latitude()
    { return mAt[0]; }
    
    //! Access longitude in degree's
    RealT &longitude()
    { return mAt[1]; }
    
    //! Height in meters
    RealT &height()
    { return mAt[2]; }

    //!< Standard deviation in position values in meters
    RealT horizontalErrorBounds() const
    { return mHorizontalError; }
    
    //!< Standard deviation in height values in meters
    RealT verticalErrorBounds() const
    { return mVerticalError; }
    
    //!< Set error bounds on GPS position in meters
    void setErrorBounds(RealT horizontalErrorBounds,RealT verticalErrorBounds)
    {
      mHorizontalError = horizontalErrorBounds;
      mVerticalError = verticalErrorBounds;
    }
    
    //! Compute the differential for each axis in global Cartesian coordinates.
    bool differential(Vector<double,3> &diffLat,Vector<double,3> &diffLong,Vector<double,3> &diffHeight) const;
    
  private:
    //! Normalise GPS coordinates.
    Point<double,3> normalise() const;

    Point<double,3> mAt; //!< Numeric GPS coordinates.

    double mHorizontalError = 0.1; //!< Standard deviation in position values.
    double mVerticalError = 0.15;   //!< Standard deviation in height values.
    
    enum AngleFormT {
      AF_None,
      AF_NS,
      AF_EW
    };
    
    //! Convert a angle in degree's to text Degree's minutes seconds.
    static std::string angleToTextDMS(RealT value);
    
    //! Convert a angle in text Degree's minutes, seconds, or decimal notation
    static bool textDMSToAngle(const std::string &strValue,RealT &value,AngleFormT af);

    //! Test if a string contains N or S
    static bool usesNS(const std::string &text);
    
    //! Test if a string contains E or W
    static bool usesEW(const std::string &text);

    // The following are the parameters for the GRS84 Ellipsoid used in GPS
    static constexpr RealT m_e2 = 6.69437999014e-3;
    static constexpr RealT m_a = 6378137.0000;
    static constexpr RealT m_b = 6356752.314245;
  };


}


