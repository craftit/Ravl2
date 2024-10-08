// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26.10.1995"

#pragma once

#include "Ravl2/Image/Segmentation/CrackCode.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Index.hh"

namespace Ravl2
{

  //! @brief Boundary vertex
  //! The relationship between pixel coordinates and boundary
  //! vertex is that the boundary vertex [i,j] is the up-left corner
  //! of the pixel (i,j). The figure contains the orientation of edges too.
  //!
  //! <pre>
  //!        [i,j] <------------ [i,j+1]
  //!        |                    /^\         .
  //!        |      (i,j)          |
  //!        V                     |
  //!       [i+1,j] ---------> [i+1, j+1]
  //!
  //! </pre>

  using BoundaryVertex = Index<2>;
  using BoundaryVertex2 = BoundaryVertex;

  [[nodiscard]] inline constexpr Index<2> right(const Index<2> &pxl)
  {
    return Index<2>(pxl[0], pxl[1] + 1);
  }

  [[nodiscard]] inline constexpr Index<2> left(const Index<2> &pxl)
  {
    return Index<2>(pxl[0], pxl[1] - 1);
  }

  [[nodiscard]] inline constexpr Index<2> down(const Index<2> &pxl)
  {
    return Index<2>(pxl[0] + 1, pxl[1]);
  }

  [[nodiscard]] inline constexpr Index<2> up(const Index<2> &pxl)
  {
    return Index<2>(pxl[0] - 1, pxl[1]);
  }

  //! @brief Elementary boundary edge
  //! The class CrackC represent an elementary edge of a discrete image.
  //! This elementary edge is located between two pixels. The edge is
  //! represented by its origin and its direction. This definition implies only
  //! 4 possible directions represented by Freeman code. Four elementary cracks
  //! around a pixel are oriented counter-clockwise about the centre of
  //! the pixel. For example, the top edge points to the left and
  //! its origin is at the upper-right corner of the pixel.</p>

  class CrackC
  {
  public:
    //! Creates an crack
    //! The value is undefined.
    constexpr CrackC() noexcept = default;

    //! Create the crack with origin in the boundary vertex 'px' and with
    //! direction 'cc'.
    constexpr CrackC(const BoundaryVertex2 &px, const CrackCode &cc) noexcept
        : mAt(px),
          mCode(cc)
    {}

    //! Create the crack with origin in the boundary vertex 'px' and with
    //! direction 'cc'.
    constexpr CrackC(const BoundaryVertex2 &px, CrackCodeT cc) noexcept
        : mAt(px),
          mCode(cc)
    {}

    // Create the crack with origin in the boundary vertex 'begin' pointing
    // towards the boundary vertex 'end'. The direction is automatically
    // generated.
    constexpr CrackC(const BoundaryVertex2 &beginAt, const BoundaryVertex2 &endAt) noexcept
        : mAt(beginAt),
          mCode(CrackCodeT::CR_NODIR)
    {
      if(right(beginAt) == endAt) mCode = CrackCodeT::CR_RIGHT;
      else if(left(beginAt) == endAt)
        mCode = CrackCodeT::CR_LEFT;
      else if(up(beginAt) == endAt)
        mCode = CrackCodeT::CR_UP;
      else if(down(beginAt) == endAt)
        mCode = CrackCodeT::CR_DOWN;
    }

    constexpr CrackC &moveDown()
    {
      mAt[0]++;
      return *this;
    }

    constexpr CrackC &moveUp()
    {
      mAt[0]--;
      return *this;
    }

    constexpr CrackC &moveRight()
    {
      mAt[1]++;
      return *this;
    }

    constexpr CrackC &moveLeft()
    {
      mAt[1]--;
      return *this;
    }

    //! @brief Creates the crack which starts at one corner of the pixel 'pxl' and has the direction 'cc'.
    //! The corner of the pixel is chosen in such way that the elementary crack is an elementary crack of the pixel.
    static constexpr auto fromPixel(const Index<2> &pxl, const CrackCode &cc)
    {
      CrackC crack(pxl, cc);
      switch(cc.code()) {
        case CrackCodeT::CR_DOWN: break;
        case CrackCodeT::CR_RIGHT:
          crack.moveDown();
          break;
        case CrackCodeT::CR_UP:
          crack.moveDown().moveRight();
          break;
        case CrackCodeT::CR_LEFT:
          crack.moveRight();
          break;
        case CrackCodeT::CR_NODIR: break;
      }
      return crack;
    }

    //:------------------
    //: Logical operators.

    //! Returns true if both cracks are equivalent.
    constexpr bool operator==(const CrackC &edg) const
    {
      return (mCode == edg.mCode) && (mAt == edg.mAt);
    }

    //! Returns true if both cracks are equivalent.
    constexpr bool operator!=(const CrackC &edg) const
    {
      return (mCode != edg.mCode) || (mAt != edg.mAt);
    }

    //:-----------------------------------------
    //: Access to elements of an elementary crack.

    //! Returns the pixel on the right side of the crack.
    [[nodiscard]] constexpr auto rightPixel() const
    {
      switch(mCode.code()) {
        case CrackCodeT::CR_DOWN: return Index<2>(mAt[0], mAt[1] - 1);
        case CrackCodeT::CR_RIGHT: break;
        case CrackCodeT::CR_UP: return Index<2>(mAt[0] - 1, mAt[1]);
        case CrackCodeT::CR_LEFT: return Index<2>(mAt[0] - 1, mAt[1] - 1);
        case CrackCodeT::CR_NODIR: break;
      }
      return mAt;
    }

    //! Returns the pixel on the left side of the crack.
    [[nodiscard]] constexpr auto leftPixel() const
    {
      switch(mCode.code()) {
        case CrackCodeT::CR_DOWN: break;
        case CrackCodeT::CR_RIGHT: return Index<2>(mAt[0] - 1, mAt[1]);
        case CrackCodeT::CR_UP: return Index<2>(mAt[0] - 1, mAt[1] - 1);
        case CrackCodeT::CR_LEFT: return Index<2>(mAt[0], mAt[1] - 1);
        case CrackCodeT::CR_NODIR: break;
      }
      return mAt;
    }

    //! Mid point along crack.
    template <typename RealT>
    [[nodiscard]] constexpr Point<RealT, 2> MidPoint() const
    {
      switch(mCode.code()) {
        case CrackCodeT::CR_DOWN: return Point<RealT, 2>({RealT(mAt[0]) + RealT(0.5), RealT(mAt[1])});
        case CrackCodeT::CR_RIGHT: return Point<RealT, 2>({RealT(mAt[0]), RealT(mAt[1]) + RealT(0.5)});
        case CrackCodeT::CR_UP: return Point<RealT, 2>({RealT(mAt[0]) - RealT(0.5), RealT(mAt[1])});
        case CrackCodeT::CR_LEFT: return Point<RealT, 2>({RealT(mAt[0]), RealT(mAt[1]) - RealT(0.5)});
        case CrackCodeT::CR_NODIR: return Point<RealT, 2>({RealT(mAt[0]) + RealT(0.5), RealT(mAt[1]) + RealT(0.5)});
      }
      assert(false);
      return Point<RealT, 2>({0, 0});
    }

    //! Returns the boundary vertex from which the elementary crack starts from.
    [[nodiscard]] constexpr const BoundaryVertex2 &vertexBegin() const
    {
      return mAt;
    }

    //! Returns the boundary vertex from which the elementary crack starts from.
    constexpr BoundaryVertex2 &vertexBegin()
    {
      return mAt;
    }

    //! Returns the boundary vertex to which the elementary crack points to.
    [[nodiscard]] constexpr BoundaryVertex2 vertexEnd() const
    {
      return crackStep(mAt, mCode.code());
    }

    //! reverse the direction of this crack.
    constexpr const CrackC &reverse()
    {
      mAt = crackStep(mAt, mCode.code());
      mCode.turnBack();
      return *this;
    }

    //! reverse the direction of this crack.
    [[nodiscard]] constexpr CrackC reversed() const
    {
      CrackCode cc = mCode;
      cc.turnBack();
      return {crackStep(mAt, mCode.code()), cc};
    }

    [[nodiscard]] constexpr BoundaryVertex2 &at()
    {
      return mAt;
    }

    [[nodiscard]] constexpr const BoundaryVertex2 &at() const
    {
      return mAt;
    }

    [[nodiscard]] constexpr CrackCode &code()
    {
      return mCode;
    }

    [[nodiscard]] constexpr const CrackCode &code() const
    {
      return mCode;
    }

    [[nodiscard]] constexpr CrackCodeT crackCode() const
    {
      return mCode.code();
    }

    //! Turns the crack code clockwise.
    // This is an in-place operation.
    inline constexpr auto &turnClock()
    {
      mCode.turnClock();
      return *this;
    }

    //! Turns the crack code counterclockwise.
    // This is an in-place operation.
    constexpr auto &turnCClock()
    {
      mCode.turnCClock();
      return *this;
    }

    //! Turns the crack code backward.
    // This is an in-place operation.
    inline constexpr auto &turnBack()
    {
      mCode.turnBack();
      return *this;
    }

  protected:
    BoundaryVertex2 mAt;
    CrackCode mCode;
  };

  //! Writes the elementary crack 'e' into the output stream 's'.
  std::ostream &operator<<(std::ostream &s, const CrackC &crack);
  std::istream &operator>>(std::istream &s, CrackC &crack);

}// namespace Ravl2

namespace fmt
{
  template <>
  struct formatter<Ravl2::CrackC> : ostream_formatter {
  };
}// namespace fmt

// Custom specialization of std::hash injected in namespace std.

template <>
struct std::hash<Ravl2::CrackC> {
  std::size_t operator()(const Ravl2::CrackC &s) const noexcept
  {
    auto ret = size_t(s.at()[0]);
    ret ^= size_t(s.at()[1]) << 4;
    ret ^= size_t(s.code().code()) << 12;
    return ret;
  }
};
