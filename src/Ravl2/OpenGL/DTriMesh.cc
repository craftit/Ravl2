// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//////////////////////////////////////////

#include <GL/gl.h>
#include "Ravl2/OpenGL/DTriMesh.hh"
#include "Ravl2/OpenGL/Canvas3D.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif


namespace Ravl2
{

  //: Constructor.
  DTriMesh::DTriMesh(const std::shared_ptr<TriMesh<RealT> > &oTriMesh)
    : model(oTriMesh)
  {
    ComputeInfo();
  }

  //: Compute center and extent of mesh.
  void DTriMesh::ComputeInfo()
  {
    if(!model)
      return;
    center = model->Centroid();
    extent = 0;

    for(auto const &vert : model->Vertices()) {
      RealT dist = euclidDistance(vert.position(), center);
      if(dist > extent)
	extent = dist;
    }
    ONDEBUG(std::cerr << "Center=" << center << " Extent=" << extent << "\n");
  }


  //: Get center of object.
  // defaults to 0,0,0
  Vector<float,3> DTriMesh::GUICenter() const
  {
    //cerr << "DTriMesh::GUICenter(): " << center << std::endl;
    return center;
  }

  //: Get extent of object.
  // defaults to 1
  float DTriMesh::GUIExtent() const
  {
    //cerr << "DTriMesh::GUIExtent(): " << extent << std::endl;
    return extent;
  }

  bool DTriMesh::GUIInit(Canvas3D &c3d)
  {
    mShaderProgram = c3d.getShaderProgram("triMesh", [&]() {
      // vertex shader
      std::string vertex_source = R"(
      )";

      return std::make_shared<GLShaderProgram>(vertex_source, "");
    });

    return true;
  }


  //: Render object.
  bool DTriMesh::GUIRender(Canvas3D &canvas) const
  {
    (void) canvas;
    if(!model)
      return true; // Don't do anything.


#if 0
    // Setup materials and colours as appropriate
    if(canvas.GetLightingMode()) {
      GLfloat ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
      GLfloat diffuse[] = {0.9f, 0.9f, 0.9f, 1.0f};
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    } else {
      glColor3f(1.0f, 1.0f, 1.0f);
    }
    // Render
    Canvas3DRenderMode eMode = canvas.GetRenderMode();
    const std::vector<Vertex<RealT>> &verts = model->Vertices();

    if(mUseMeshColour) {
      glEnable(GL_COLOR_MATERIAL);
      glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    }

    switch(eMode) {
      case C3D_SMOOTH:
      case C3D_POINT:
      case C3D_WIRE:glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(Vertex<RealT>), reinterpret_cast<const void *>(&(verts[0].normal())));
	// Fall through
	FMT_FALLTHROUGH;
	case C3D_FLAT:glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex<RealT>), reinterpret_cast<const void *>(&(verts[0].position())));
	break;
    }

    switch(eMode) {
      case C3D_POINT: {
	// Draw individual points
	glDrawArrays(GL_POINTS, 0, GLsizei(verts.size()));
      }
	break;
      case C3D_WIRE: {
	for(auto const &it : model->Faces()) {
	  glBegin(GL_LINE);
	  glArrayElement(GLint(model->index(it, 0)));
	  glArrayElement(GLint(model->index(it, 1)));
	  glArrayElement(GLint(model->index(it, 2)));
	  glEnd();
	}
      } break;
      case C3D_FLAT: {
	ONDEBUG(std::cerr << "flat render. \n");
	int eGLShadeModel = 0;
	glGetIntegerv(GL_SHADE_MODEL, &eGLShadeModel);
	glShadeModel(GL_FLAT); // Flat shading
	// Draw filled polygon
	for(auto const &it : model->Faces()) {
	  if(mUseMeshColour) {
	    GLColour(it.Colour());
	  }
	  GLNormal(it.FaceNormal());
	  glBegin(GL_POLYGON);
	  glArrayElement(GLint(model->index(it, 0)));
	  glArrayElement(GLint(model->index(it, 1)));
	  glArrayElement(GLint(model->index(it, 2)));
	  glEnd();
	}
	glShadeModel(GLenum(eGLShadeModel)); // Restore old shade model
      }	 break;
      case C3D_SMOOTH: {
	ONDEBUG(std::cerr << "Smooth render. \n");
	int eGLShadeModel = 0;
	glGetIntegerv(GL_SHADE_MODEL, &eGLShadeModel);
	glShadeModel(GL_SMOOTH); // Flat shading
	// Draw filled polygon
	for(auto const &it : model->Faces()) {
	  if(mUseMeshColour) {
	    GLColour(it.Colour());
	  }
	  glBegin(GL_POLYGON);
	  glArrayElement(GLint(model->index(it, 0)));
	  glArrayElement(GLint(model->index(it, 1)));
	  glArrayElement(GLint(model->index(it, 2)));
	  glEnd();
	}
	glShadeModel(GLenum(eGLShadeModel)); // Restore old shade model
      } break;
    };

    switch(eMode) {
      case C3D_SMOOTH:
      case C3D_POINT:
      case C3D_WIRE:glDisableClientState(GL_NORMAL_ARRAY);
        // Fall through
      FMT_FALLTHROUGH; case C3D_FLAT:glDisableClientState(GL_VERTEX_ARRAY); break;
	break;
    }
    if(mUseMeshColour){
      glDisable(GL_COLOR_MATERIAL);
   }
#endif
    return true;
  }

}

