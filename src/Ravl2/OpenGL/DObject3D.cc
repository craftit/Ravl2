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
  bool DObject3D::GUIRenderDL(Canvas3D &c3d) {
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


  /// DObjectSet3DBodyC ///////////////////////////////////////////////

  //: Render object.
  bool DObjectSet3DBodyC::GUIRender(Canvas3D &c3d) const
  {
    //cerr << "DObjectSet3DBodyC::GUIRender\n";
    for(auto &x: parts)
      x->GUIRender(c3d);
    return true;
  }

  //: Get center of object.
  // defaults to 0,0,0
  Vector<float,3> DObjectSet3DBodyC::GUICenter() const
  {
    assert(!mUpdateNeeded);
    return center;
  }

  //: Get extent of object.
  // defaults to 1
  float DObjectSet3DBodyC::GUIExtent() const
  {
    assert(!mUpdateNeeded);
    return extent;
  }
  
  //: Add object into list.
  void DObjectSet3DBodyC::GUIAdd(const std::shared_ptr<DObject3D> &obj) {
    parts.push_back(obj);
    center = (center * (parts.size() - 1) + obj->GUICenter()) / parts.size();
    mUpdateNeeded = true;
  }
  
  void DObjectSet3DBodyC::UpdateCenterExtent()
  {
    center = toVector<float>(0,0,0);
    for(auto &x: parts) {
      center += x->GUICenter();
    }
    center /= float(parts.size());
    extent = 0;
    for(auto &x: parts) {
      float distToCenter = float((x->GUICenter() - center).norm() + x->GUIExtent());
      if(distToCenter > extent) {
        extent = distToCenter;
      }
    }
    mUpdateNeeded = false;
  }
  
  
  //// DOpenGLBodyC ////////////////////////////////////////////////////////
  
  //: Render object.
  bool DOpenGLBodyC::GUIRender(Canvas3D &c3d) const
  {
    //cerr << "DOpenGLBodyC::GUIRender\n";
    if(sigEvent)
      sigEvent(c3d);
    return true;
  }

}

