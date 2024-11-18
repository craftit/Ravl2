// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//////////////////////////////////////////

#include "Ravl2/OpenGL/DTexTriMesh3D.hh"
#include "Ravl2/OpenGL/Canvas3D.hh"
#include "Ravl2/Image/WarpScale.hh"
#include "GL/gl.h"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

#define USEMESHCOLOUR 0

namespace Ravl2 {

  DTexTriMesh3DBodyC::DTexTriMesh3DBodyC(const std::shared_ptr<TexTriMesh<RealT> > &oTexTriMesh)
    :  DTriMesh3D(oTexTriMesh),
       tmodel(oTexTriMesh),
       texNames(0)
  {}
  //: Constructor.

  DTexTriMesh3DBodyC::~DTexTriMesh3DBodyC()
  {
    // Make sure textures are free'd in an appropriate context.
    if(m_glContext && texNames.size() > 0)
      m_glContext->FreeTextures(texNames);
  }
  
//  static int PowerOfTwo(int Val)
//  {
//    Val--;
//    for(int i = 31; i > 0; i--)
//      if(Val >> i)
//        return 1 << (i+1);
//    return 0;
//  }
  
  //: Render object.
  bool DTexTriMesh3DBodyC::GUIRender(Canvas3D &canvas) const
  {
    (void)canvas;
    ONDEBUG(std::cerr << "DTexTriMesh3DBodyC::GUIRender\n");
    if(!tmodel)
      return false;
#if 0
    // Setup GL texturing if it's not already done
    if (texNames.size() == 0 && tmodel->NumTextures() != 0) {
      ONDEBUG(std::cerr << "creating tex names\n");

      //roate texture coordinates
      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();
      glRotated(-90, 0., 0., 1.);

      // Not sure what this line does...
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      // Allocate textures
      texNames = std::vector<unsigned int>(tmodel->NumTextures());
      
      // Remeber the context we about to allocate the textures in
      m_glContext = canvas.GUIGLContext();
      
      // Create texture name
      glGenTextures(tmodel->NumTextures(), &(texNames[0]));
      for(int i = 0; i < tmodel.NumTextures(); i++)
      {
        //cerr << "texture:" << i << std::endl;
        glBindTexture(GL_TEXTURE_2D, texNames[i]);
        // Setup texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        // Setup texture image
        const Array<PixelRGB8,2> &curTexture = tmodel.Textures()[i];
        //cerr << "size:" << curTexture.range(1).size() << "  " << curTexture.range(0).size() << std::endl;

        int newRows = PowerOfTwo(curTexture.range(0).size());
        int newCols = PowerOfTwo(curTexture.range(1).size());
        
        if(canvas.HaveExtNonPowerOfTwoTexture() || ((int) curTexture.range(0).size() == newRows &&
                                                    (int) curTexture.range(1).size() == newCols)) {
          // We don't care what size the texture is, we can use it.
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                       curTexture.range(1).size(), curTexture.range(0).size(),
                       0, GL_RGB, GL_UNSIGNED_BYTE,
                       (void *)(curTexture.Row(curTexture.min(0))));
          
        } else {
          
          // Create texture with power of two size
          //cerr << "size:" << newCols << "  " << newRows << std::endl;
          Array<PixelRGB8,2> texture =
            WarpScaleC<PixelRGB8, PixelRGB8>(IndexRange<2>(newRows, newCols)).
            Apply(curTexture);
          //cerr << "size:" << texture.range(1).size() << "  " << texture.range(0).size() << std::endl;
          
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                       texture.range(1).size(), texture.range(0).size(),
                       0, GL_RGB, GL_UNSIGNED_BYTE,
                       (void *)(texture.Row(texture.min(0))));
        }
      }
      //cerr << "tex names ok\n";
    }

    // Setup materials and colours as appropriate
    if (canvas.GetLightingMode()) {
      GLfloat ambient[]  = {0.2f,0.2f,0.2f,1.0f};
      GLfloat diffuse[]  = {0.9f,0.9f,0.9f,1.0f};
      glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,ambient);
      glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse);
    } else {
      glColor3f(1.0f,1.0f,1.0f);
    }
    // Render
    Canvas3DRenderMode eMode = canvas.GetRenderMode();
    const std::vector<Vertex<RealT>> &verts = tmodel.Vertices();

#if USEMESHCOLOUR
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
#endif

    // Enable and set up texturing
    if(canvas.GetTextureMode())
    {
      //cerr << "enabling texture\n";
      glEnable(GL_TEXTURE_2D);
      //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    }

    switch(eMode) {
    case C3D_SMOOTH:
    case C3D_POINT:
    case C3D_WIRE:
      //cerr << "set normals\n";
      //FIXME
      //glEnableClientState(GL_NORMAL_ARRAY);
      //glNormalPointer(GL_DOUBLE, sizeof(Vertex<RealT>), (void *)&(verts[0].Normal()));
      //break;
    case C3D_FLAT:
      //cerr << "set vertices\n";
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(3,GL_DOUBLE,sizeof(Vertex<RealT>),(void *)&(verts[0].position()));
      break;
    }

    switch(eMode)
    {
    case C3D_POINT:
      // Draw individual points
      glDrawArrays(GL_POINTS,0,verts.size());
      break;
    case C3D_WIRE:
      for(SArray1dIterC<Tri<RealT>> it(tmodel.Faces());it;it++) {
        glBindTexture(GL_TEXTURE_2D,texNames[it->TextureID()]);
        glBegin(GL_LINE_LOOP);
        GLTexCoord(it->TextureCoord(0));
        glArrayElement(tmodel.Index(*it,0));
        GLTexCoord(it->TextureCoord(1));
        glArrayElement(tmodel.Index(*it,1));
        GLTexCoord(it->TextureCoord(2));
        glArrayElement(tmodel.Index(*it,2));
        glEnd();
      }
      break;
    case C3D_FLAT: {
      ONDEBUG(std::cerr << "flat render. \n");
      int eGLShadeModel;
      glGetIntegerv(GL_SHADE_MODEL,&eGLShadeModel);
      glShadeModel(GL_FLAT); // Flat shading
      // Draw filled polygon
      for(SArray1dIterC<Tri<RealT>> it(tmodel.Faces());it;it++) {
#if USEMESHCOLOUR
        glColor3ubv(&(it->Colour()[0]));
#endif
        GLNormal(it->FaceNormal());
        //cerr << "textureid:" << int(it->TextureID()) << std::endl;
        glBindTexture(GL_TEXTURE_2D, texNames[it->TextureID()]);
        glBegin(GL_POLYGON);
        //cerr << "tc: " << it->TextureCoord(0) << "  " << it->TextureCoord(1) << "  " << it->TextureCoord(2) << std::endl;
        GLTexCoord(it->TextureCoord(0));
        glArrayElement(tmodel.Index(*it, 0));
        GLTexCoord(it->TextureCoord(1));
        glArrayElement(tmodel.Index(*it, 1));
        GLTexCoord(it->TextureCoord(2));
        glArrayElement(tmodel.Index(*it, 2));
        glEnd();
      }
      glShadeModel((GLenum)eGLShadeModel); // Restore old shade model
    } break;
    case C3D_SMOOTH: {
      ONDEBUG(std::cerr << "Smooth render. \n");
      int eGLShadeModel;
      glGetIntegerv(GL_SHADE_MODEL, &eGLShadeModel);
      glShadeModel(GL_SMOOTH); // smooth shading
      // Draw filled polygon
      for(SArray1dIterC<Tri<RealT>> it(model.Faces()); it; it++)
      {
#if USEMESHCOLOUR
        glColor3ubv(&(it->Colour()[0]));
#endif
        //cerr << "textureid:" << int(it->TextureID()) << std::endl;
        glBindTexture(GL_TEXTURE_2D, texNames[it->TextureID()]);
        //glBegin(GL_POLYGON);
        glBegin(GL_TRIANGLES);
        //cerr << "tc: " << it->TextureCoord(0) << "  " << it->TextureCoord(1) << "  " << it->TextureCoord(2) << std::endl;
        GLTexCoord(it->TextureCoord(0));
        glArrayElement(tmodel.Index(*it, 0));
        GLTexCoord(it->TextureCoord(1));
        glArrayElement(tmodel.Index(*it, 1));
        GLTexCoord(it->TextureCoord(2));
        glArrayElement(tmodel.Index(*it, 2));
        glEnd();
      }
      glShadeModel((GLenum)eGLShadeModel); // Restore old shade model
    } break;
    };

    // Disable texturing
    if (canvas.GetTextureMode()) {
      glDisable(GL_TEXTURE_2D);
    }

    // Disable arrays
    switch(eMode) {
    case C3D_SMOOTH:
    case C3D_POINT:
    case C3D_WIRE:
      glDisableClientState(GL_NORMAL_ARRAY);
      /* no break */
    case C3D_FLAT:
      glDisableClientState(GL_VERTEX_ARRAY);
      break;
    }
#if USEMESHCOLOUR
    glDisable(GL_COLOR_MATERIAL);
#endif
#endif
    return true;
  }

}

