// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001-12, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/3D/PinholeCamera2.hh"

namespace Ravl2
{

  //: Load from stream.
  bool
  PinholeCameraBody2C::Load(std::istream &s)
  {
    s >> 
      m_frame >>
      m_fx >> 
      m_fy >> 
      m_cx >> 
      m_cy >> 
      m_sizex >> 
      m_sizey >> 
      m_k3 >> 
      m_k5 >> 
      m_R  >> 
      m_t;
    return true;
  }
    
  //: Load from binary stream.
  bool
  PinholeCameraBody2C::Load(BinIStreamC &s)
  {
    s >> 
      m_frame >>
      m_fx >> 
      m_fy >> 
      m_cx >> 
      m_cy >> 
      m_sizex >> 
      m_sizey >> 
      m_k3 >> 
      m_k5 >> 
      m_R  >> 
      m_t;
    return true;
  }
    
  //: Writes object to stream, can be loaded using constructor
  bool 
  PinholeCameraBody2C::Save(std::ostream &s) const
  {
    s << m_frame
      << endl << m_fx 
      << " " << m_fy
      << " " << m_cx
      << " " << m_cy
      << " " << m_sizex
      << " " << m_sizey
      << endl;
    s << m_k3 << " " << m_k5 << endl; 
    s << m_R;
    s << m_t << endl;
    return true;
  }
    
  //: Writes object to stream, can be loaded using constructor
  bool 
  PinholeCameraBody2C::Save(BinOStreamC &s) const
  {
    s << m_frame
      << m_fx 
      << m_fy 
      << m_cx 
      << m_cy 
      << m_sizex
      << m_sizey
      << m_k3
      << m_k5
      << m_R
      << m_t;
    return true;
  }
  
};
