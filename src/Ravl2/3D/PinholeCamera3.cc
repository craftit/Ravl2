// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001-12, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#include "Ravl2/3D/PinholeCamera3.hh"

namespace Ravl3DN
{

#if 0
  //: Load from stream.
  bool
  PinholeCameraBody3C::Load(std::istream &s)
  {
    s >> 
      m_frame >>
      m_fx >> 
      m_fy >> 
      m_cx >> 
      m_cy >> 
      m_k1 >> 
      m_k2 >> 
      m_R  >> 
      m_t;
    return true;
  }
    
  //: Load from binary stream.
  bool
  PinholeCameraBody3C::Load(BinIStreamC &s)
  {
    s >> 
      m_frame >>
      m_fx >> 
      m_fy >> 
      m_cx >> 
      m_cy >> 
      m_k1 >> 
      m_k2 >> 
      m_R  >> 
      m_t;
    return true;
  }
    
  //: Writes object to stream, can be loaded using constructor
  bool 
  PinholeCameraBody3C::Save(std::ostream &s) const
  {
    s << m_frame
      << endl << m_fx 
      << " " << m_fy
      << " " << m_cx
      << " " << m_cy
      << endl;
    s << m_k1 << " " << m_k2 << " " << endl; 
    s << m_R;
    s << m_t << endl;
    return true;
  }
    
  //: Writes object to stream, can be loaded using constructor
  bool 
  PinholeCameraBody3C::Save(BinOStreamC &s) const
  {
    s << m_frame
      << m_fx 
      << m_fy 
      << m_cx 
      << m_cy 
      << m_k1
      << m_k2
      << m_R
      << m_t;
    return true;
  }
#endif
};
