// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/OpenGL/DObject3D.hh"

namespace Ravl2
{


  //: Render, checking for display lists.
  bool DObject3DBodyC::GUIRenderDL(Canvas3DC &c3d) {
    bool ret = false;
    //cerr << "Calling Genlist. " << id << "\n";
    if(m_useDisplayList) {
      if(id < std::numeric_limits<GLuint>::max()) {
        //cerr << "Calling Genlist. " << id << "\n";
        glCallList(id);
      } else {// Generate display list.
        id = glGenLists(1);
        //cerr << "New Genlist. " << id << "\n";
        glNewList(id, GL_COMPILE);
        ret = GUIRender(c3d);
        glEndList();
        glCallList(id);
      }
    } else {
      ret = GUIRender(c3d);
    }
    // Don't use a display list...
    return ret;
  }

#if 0

  /// DObjectSet3DBodyC ///////////////////////////////////////////////
  //: Default constructor.
  DObjectSet3DBodyC::DObjectSet3DBodyC()
    : gotCenter(false),
      center(0,0,0),
      gotExtent(false),
      extent(1)
  {}

  //: Render object.
  bool DObjectSet3DBodyC::GUIRender(Canvas3DC &c3d) const
  {
    //cerr << "DObjectSet3DBodyC::GUIRender\n";
    for(DLIterC<DObject3DC> it(parts);it.valid();it.next())
      it.Data().GUIRender(c3d);
    return true;
  }

  //: Get center of object.
  // defaults to 0,0,0
  Vector<RealT,3> DObjectSet3DBodyC::GUICenter() const
  {
    if(!gotCenter)
    {
      center = Vector<RealT,3>(0, 0, 0);
      if(parts.size() != 0)
      {
        RealT count = 0;
        for(DLIterC<DObject3DC> it(parts); it; it++, count++)
          center += it.Data().GUICenter();
        center /= count;
      }
      gotCenter = true;
    }
    return center;
  }

  //: Get extent of object.
  // defaults to 1
  RealT DObjectSet3DBodyC::GUIExtent() const
  {
    if(!gotExtent)
    {
      extent = 1;
      if(parts.size() != 0)
      {
        Vector<RealT,3> at = GUICenter();
        RealT maxDist = 0;
        for(DLIterC<DObject3DC> it(parts); it; it++)
        {
          RealT dist = at.EuclidDistance(it.Data().GUICenter()) +
                                         it.Data().GUIExtent();
          if(dist > maxDist)
            maxDist = dist;
        }
        extent = maxDist;
      }
      gotExtent = true;
    }
    return extent;
  }


  //// DOpenGLBodyC ////////////////////////////////////////////////////////
  //: Render object.
  bool DOpenGLBodyC::GUIRender(Canvas3DC &c3d) const
  {
    //cerr << "DOpenGLBodyC::GUIRender\n";
    if(sigEvent.IsValid())
      sigEvent.Invoke();
    return true;
  }

#endif
}

