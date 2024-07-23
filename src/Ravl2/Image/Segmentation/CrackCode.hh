// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26.10.1992"

#pragma once

#include <array>
#include "Ravl2/Index.hh"

namespace Ravl2
{
  
  //! Crack code
  // Symbol names of crack code, ordered counter-clockwise.
  
  enum class CrackCodeT : int {
    CR_DOWN  = 0,
    CR_RIGHT = 1,
    CR_UP    = 2,
    CR_LEFT  = 3,
    CR_NODIR = 4
  };
  
  //! Relative crack code
  // Symbol names of crack code, ordered counter-clockwise.
  
  enum class RelativeCrackCodeT : int {
    CR_AHEAD     = 0,
    CR_TO_LEFT   = 1,
    CR_BACK      = 2,
    CR_TO_RIGHT  = 3,
    CR_RNODIR    = 4
  };

  constexpr Index<2> CrackStep(const Index<2> &pixel,CrackCodeT crackCode);
  constexpr Index<2> CrackDirection(CrackCodeT crackCode);

  //! Crack code or Freeman code
  
  class CrackCodeC {
  public:
    //! Default constructor.
    // Direction is undefined.
    constexpr CrackCodeC()
    = default;

    //! Constructs object from an integer 'i'.
    constexpr explicit CrackCodeC(int i)
     : crackCode(CrackCodeT(i))
    {
      assert(i >= 0 && i <= 4);
    }

    //! Constructs and set value to be 'cc'.
    constexpr explicit CrackCodeC(CrackCodeT cc)
      : crackCode(cc)
    {}

    //! Returns the crack code.
    [[nodiscard]] inline constexpr CrackCodeT Code() const
    { return crackCode; }

    //! Get relative crack code of direction 'cc' relative to this one.
    [[nodiscard]] constexpr RelativeCrackCodeT Relative(const CrackCodeC & cc) const {
      int rcode = int(cc.Code()) - int(Code());
      if(rcode < 0) rcode += 4;
      else rcode %= 4;
      return static_cast<RelativeCrackCodeT>(rcode); 
    }

    //! Equality test
    [[nodiscard]] inline constexpr bool operator==(const CrackCodeC & cc) const
    { return cc.Code() == Code(); }

    [[nodiscard]] inline constexpr bool operator!=(const CrackCodeC & cc) const
    { return (bool)(cc.Code() != Code()); }
    //: Returns true if the object content is not equal to 'cc'.
    
    inline constexpr const CrackCodeC & operator+=(const CrackCodeC & cc)  {
      int result = int(crackCode) + int(cc.crackCode);
      crackCode = CrackCodeT(result % 4);
      return(*this);
    }
    //: Add a relative crack code.
    // The crack code 'cc' is taken as a relative crack code. The relative
    // crack code is added to this crack code.

    inline constexpr const CrackCodeC & operator-=(const CrackCodeC & cc) {
      int result = int(crackCode) - int(cc.crackCode) + 4;
      crackCode = CrackCodeT(result % 4);
      return *this;
    }
    //: Subtract a relative crack code.
    // The crack code 'cc' is taken as a relative crack code. 
    // The relative crack code is subtracted from this crack code.

    constexpr const CrackCodeC &operator=(const CrackCodeC & cc) {
      crackCode=cc.crackCode;
      return *this;
    }
    //: Assignment.

    constexpr const CrackCodeC &operator=(const CrackCodeT & cc) {
      crackCode=cc;
      return *this;
    }
    //: Assignment.
    
    // ----------   turning -----------------------

    //1 Turns the crack code clockwise.
    // This is an in-place operation.
    inline constexpr CrackCodeC & TurnClock()
    { crackCode = clockWiseTurn[int(crackCode)]; return *this; }

    //! Turns the crack code counterclockwise.
    // This is an in-place operation.
    inline constexpr CrackCodeC & TurnCClock()
    { crackCode = cClockWiseTurn[int(crackCode)]; return *this; }

    //! Turns the crack code backward.
    // This is an in-place operation.
    inline constexpr CrackCodeC & TurnBack()
    { crackCode = backTurn[int(crackCode)]; return *this; }

    //! Get pixel in the direction of the crack code.
    [[nodiscard]] constexpr Index<2> Next(const Index<2> &pixel) const
    { return pixel + offset[int(crackCode)]; }

  protected:

    constexpr static const CrackCodeT clockWiseTurn[5]  =
        {CrackCodeT::CR_LEFT, CrackCodeT::CR_DOWN, CrackCodeT::CR_RIGHT, CrackCodeT::CR_UP, CrackCodeT::CR_NODIR};

    constexpr static const CrackCodeT cClockWiseTurn[5] =
        {CrackCodeT::CR_RIGHT, CrackCodeT::CR_UP, CrackCodeT::CR_LEFT, CrackCodeT::CR_DOWN, CrackCodeT::CR_NODIR};

    constexpr static const CrackCodeT backTurn[5] =
        { CrackCodeT::CR_UP, CrackCodeT::CR_LEFT, CrackCodeT::CR_DOWN, CrackCodeT::CR_RIGHT,CrackCodeT::CR_NODIR};

    constexpr static const std::array<Index<2>,5> offset = { Index<2>( {1, 0}), Index<2>( {0, 1}), Index<2>({-1, 0}), Index<2>({ 0,-1}), Index<2>( {0, 0}) };

    CrackCodeT crackCode = CrackCodeT::CR_NODIR;  // The code.
    friend constexpr Index<2> CrackStep(const Index<2> & ,CrackCodeT );
    friend constexpr Index<2> CrackDirection(CrackCodeT );
  };

  //! Write to a stream.
  inline std::ostream &operator<<(std::ostream &strm,const CrackCodeC &cc) {
    strm << (int) cc.Code();
    return strm;
  }

  //! Read from a stream.
  inline std::istream &operator>>(std::istream &strm,CrackCodeC &cc) {
    int v;
    strm >> v;
    cc = (CrackCodeT) v;
    return strm;
  }

  //! Step one pixel in the direction of the crack code.
  inline
  constexpr Index<2> CrackStep(const Index<2> &pixel,CrackCodeT crackCode)
  { return pixel + CrackCodeC::offset[int(crackCode)]; }

  //! Direction in the form of an offset for a crack code.
  inline
  constexpr Index<2> CrackDirection(CrackCodeT crackCode)
  { return  CrackCodeC::offset[int(crackCode)]; }

}
