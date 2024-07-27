// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! date="26/10/1992"
//! author="Radek Marik"

#include "Ravl2/Image/Segmentation/CrackCode.hh"

namespace Ravl2
{
  std::string_view toString(CrackCodeT cc)
  {
    switch(cc) {
    case CrackCodeT::CR_DOWN:
      return "DOWN";
    case CrackCodeT::CR_RIGHT:
      return "RIGHT";
    case CrackCodeT::CR_UP:
      return "UP";
    case CrackCodeT::CR_LEFT:
      return "LEFT";
    case CrackCodeT::CR_NODIR:
      return "NODIR";
    default:
      return "Unknown";
    }
  }

  std::string_view toString(RelativeCrackCodeT cc)
  {
    switch(cc) {
    case RelativeCrackCodeT::CR_AHEAD:
      return "AHEAD";
    case RelativeCrackCodeT::CR_TO_LEFT:
      return "TO_LEFT";
    case RelativeCrackCodeT::CR_BACK:
      return "BACK";
    case RelativeCrackCodeT::CR_TO_RIGHT:
      return "TO_RIGHT";
    case RelativeCrackCodeT::CR_RNODIR:
      return "RNODIR";
    default:
      return "Unknown";
    }
  }

  [[nodiscard]] bool fromString(std::string_view str, CrackCodeT &cc)
  {
    if(str == "DOWN") {
      cc = CrackCodeT::CR_DOWN;
      return true;
    }
    if(str == "RIGHT") {
      cc = CrackCodeT::CR_RIGHT;
      return true;
    }
    if(str == "UP") {
      cc = CrackCodeT::CR_UP;
      return true;
    }
    if(str == "LEFT") {
      cc = CrackCodeT::CR_LEFT;
      return true;
    }
    if(str == "NODIR") {
      cc = CrackCodeT::CR_NODIR;
      return true;
    }
    return false;
  }

  [[nodiscard]] bool fromString(std::string_view str, RelativeCrackCodeT &cc)
  {
    if(str == "AHEAD") {
      cc = RelativeCrackCodeT::CR_AHEAD;
      return true;
    }
    if(str == "TO_LEFT") {
      cc = RelativeCrackCodeT::CR_TO_LEFT;
      return true;
    }
    if(str == "BACK") {
      cc = RelativeCrackCodeT::CR_BACK;
      return true;
    }
    if(str == "TO_RIGHT") {
      cc = RelativeCrackCodeT::CR_TO_RIGHT;
      return true;
    }
    if(str == "RNODIR") {
      cc = RelativeCrackCodeT::CR_RNODIR;
      return true;
    }
    return false;
  }







}
