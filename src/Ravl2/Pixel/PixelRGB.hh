// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="24/01/2001"

#pragma once

#include <array>
#include "Ravl2/Types.hh"
#include "Ravl2/Pixel/Pixel.hh"

namespace Ravl2
{

  //: RGB Pixel base class, this is for convenience only.

  template <class CompT>
  class PixelRGB : public Pixel<CompT, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>
  {
  public:
    //! Default constructor.
    // Creates an undefined value.
    PixelRGB() = default;

    //! Construct from component values.
    PixelRGB(const CompT &r, const CompT &g, const CompT &b)
    {
      (*this)[0] = r;
      (*this)[1] = g;
      (*this)[2] = b;
    }

    //! Construct from another component type.
    template <class OCompT>
    explicit PixelRGB(const PixelRGB<OCompT> &oth)
    {
      (*this)[0] = CompT(oth.Red());
      (*this)[1] = CompT(oth.Green());
      (*this)[2] = CompT(oth.Blue());
    }

    //! Constructor from base class.
    explicit PixelRGB(const Vector<CompT, 3> &v)
        : Vector<CompT, 3>(v)
    {}

    //! Set the values.
    void Set(const CompT &r, const CompT &g, const CompT &b)
    {
      (*this)[0] = r;
      (*this)[1] = g;
      (*this)[2] = b;
    }

    //! Returns the level of the red component.
    [[nodiscard]] inline const CompT &Red() const
    {
      return (*this)[0];
    }

    //! Returns the level of the green component.
    [[nodiscard]] inline const CompT &Green() const
    {
      return (*this)[1];
    }

    //! Returns the level of the blue component.
    [[nodiscard]] inline const CompT &Blue() const
    {
      return (*this)[2];
    }

    //! Returns the level of the red component.
    [[nodiscard]] inline CompT &Red()
    {
      return (*this)[0];
    }

    //! Returns the level of the green component.
    [[nodiscard]] inline CompT &Green()
    {
      return (*this)[1];
    }

    //! Returns the level of the blue component.
    [[nodiscard]] inline CompT &Blue()
    {
      return (*this)[2];
    }

    //! Get the pixel intensity of an NTSC colour system.
    //! the NTSC RGB color system.
    [[nodiscard]] inline CompT NTSC_Y()
    {
      return get<ImageChannel::Luminance>(*this);
    }

    //! Calculate intensity of the pixel.
    //! This returns the average of the red, green
    //! and blue components.
    [[nodiscard]] CompT Y() const
    {
      return ((*this)[0] + (*this)[1] + (*this)[2]) / 3;
    }
  };

  //! Stream input.
  template <class CompT>
  inline std::istream &operator>>(std::istream &strm, PixelRGB<CompT> &val)
  {
    return strm >> val[0] >> val[1] >> val[2];
  }

  //! Stream output.
  template <class CompT>
  inline std::ostream &operator<<(std::ostream &strm, const PixelRGB<CompT> &val)
  {
    return strm << val[0] << ' ' << val[1] << ' ' << val[2];
  }

  //! Stream input.
  // This is to make sure bytes are handled as numeric values.
  inline std::istream &operator>>(std::istream &strm, PixelRGB<uint8_t> &val);

  //! Stream output.
  // This is to make sure bytes are handled as numeric values.
  inline std::ostream &operator<<(std::ostream &strm, const PixelRGB<uint8_t> &val);

}// namespace Ravl2
