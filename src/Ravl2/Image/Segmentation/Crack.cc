// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! date="26.10.1992"
//! author="Radek Marik"


#include "Ravl2/Image/Segmentation/Crack.hh"

namespace Ravl2
{

  std::ostream & operator<<(std::ostream & s, const CrackC & crack)
  { return s << crack.at() << ' ' << toString(crack.code().code()); }
  //: Writes the elementary crack 'e' into the output stream 's'.

}


  
