// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/OpenGL/DObject.hh"

namespace Ravl2
{

  //! Initialize object.
  //! Register object with OpenGL, setup shaders, etc.
  bool DObject::GUIInit(Canvas3D &c3d)
  {
    (void)c3d;
    return true;
  }


  //: Render, checking for display lists.
  bool DObject::GUIRenderDL(Canvas3D &c3d) {
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


  /// DObjectSet3D ///////////////////////////////////////////////

  //: Render object.
  bool DObjectSet3D::GUIRender(Canvas3D &c3d) const
  {
    //cerr << "DObjectSet3DBodyC::GUIRender\n";
    for(auto &x: parts) {
      assert(x != nullptr);
      x->GUIRender(c3d);
    }
    return true;
  }

  bool DObjectSet3D::GUIInit(Canvas3D &c3d)
  {
    bool ret = true;
    for(auto &x: parts) {
      assert(x != nullptr);
      if(!x->GUIInit(c3d)) {
        ret = false;
      }
    }
    return ret;
  }

  Vector<float,3> DObjectSet3D::GUICenter() const
  {
    assert(!mUpdateNeeded);
    return center;
  }

  float DObjectSet3D::GUIExtent() const
  {
    assert(!mUpdateNeeded);
    return extent;
  }
  
  //: Add object into list.
  void DObjectSet3D::GUIAdd(const std::shared_ptr<DObject> &obj) {
    parts.push_back(obj);
    center = (center * (parts.size() - 1) + obj->GUICenter()) / parts.size();
    mUpdateNeeded = true;
  }
  
  void DObjectSet3D::UpdateCenterExtent()
  {
    center = toVector<float>(0,0,0);
    for(auto &x: parts) {
      center += x->GUICenter();
    }
    center /= float(parts.size());
    extent = 0;
    for(auto &x: parts) {
      float const distToCenter = (x->GUICenter() - center).norm() + x->GUIExtent();
      if(distToCenter > extent) {
        extent = distToCenter;
      }
    }
    mUpdateNeeded = false;
  }
  
  
  //// DOpenGLDirect ////////////////////////////////////////////////////////
  
  //: Render object.
  bool DOpenGLDirect::GUIRender(Canvas3D &c3d) const
  {
    //cerr << "DOpenGLDirect::GUIRender\n";
    if(sigEvent) {
      sigEvent(c3d);
    }
    return true;
  }

}

