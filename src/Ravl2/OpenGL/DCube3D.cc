// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
////////////////////////////////////////////

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Ravl2/OpenGL/DCube3D.hh"
#include "Ravl2/OpenGL/Canvas3D.hh"

namespace Ravl2
{
  //! Register object with OpenGL.
  bool DCube3D::GUIInit(Canvas3D &c3d)
  {
    (void)c3d;
    // Has init been called before?
    if(mShaderProgram)
      return true;

    // create shader if it doesn't exist
    mShaderProgram = c3d.getShaderProgram("cube", [&]() {
      // vertex shader
      std::string vertex_source = R"(
#version 330
uniform mat4 ViewProjection; // the projection matrix uniform
layout(location = 0) in vec4 vposition;
layout(location = 1) in vec4 vcolor;
out vec4 fcolor;
void main() {
   fcolor = vcolor;
   gl_Position = ViewProjection*vposition;
}
)";

      std::string fragment_source = R"(
#version 330
in vec4 fcolor;
layout(location = 0) out vec4 FragColor;
void main() {
   FragColor = fcolor;
}
)";
      auto program = std::make_shared<GLShaderProgram>(vertex_source, fragment_source);
      program->link();
      return program;
    });

    mShaderProgram->use();

    mViewProjectionLocation = mShaderProgram->getUniformLocation("ViewProjection");

    // generate and bind the mVao
    mVertexArray.generate(1);
    mVertexArray.bind(0);

    mVertexBuffer = std::make_shared<GLVertexBuffer>();
    mVertexBuffer->generate(2);

    mVertexBuffer->bind(GL_ARRAY_BUFFER, 0);

    // data for a cube
    static GLfloat vertexData[] = {
      //  X     Y     Z           R     G     B
      // face 0:
      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // vertex 0
      -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // vertex 1
      1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // vertex 2
      -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f,// vertex 3

      // face 1:
      1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // vertex 0
      1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // vertex 1
      1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // vertex 2
      1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f,// vertex 3

      // face 2:
      1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // vertex 0
      1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // vertex 1
      -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // vertex 2
      -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f,// vertex 3

      // face 3:
      1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f,  // vertex 0
      1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, // vertex 1
      -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f, // vertex 2
      -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f,// vertex 3

      // face 4:
      -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // vertex 0
      -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, // vertex 1
      -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // vertex 2
      -1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f,// vertex 3

      // face 5:
      1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f,  // vertex 0
      -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // vertex 1
      1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, // vertex 2
      -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f,// vertex 3
    };// 6 faces with 4 vertices with 6 components (floats)

    // fill with data
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4 * 6, vertexData, GL_STATIC_DRAW);

    // set up generic attrib pointers

    mVertexArray.addBuffer(mVertexBuffer, 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (char *)0 + 0 * sizeof(GLfloat));

    mVertexArray.addBuffer(mVertexBuffer, 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (char *)0 + 3 * sizeof(GLfloat));

    // generate and bind the index buffer object
    mVertexBuffer->bind(GL_ELEMENT_ARRAY_BUFFER, 1);

    static GLuint indexData[] = {
      // face 0:
      0, 1, 2,// first triangle
      2, 1, 3,// second triangle
      // face 1:
      4, 5, 6,// first triangle
      6, 5, 7,// second triangle
      // face 2:
      8, 9, 10, // first triangle
      10, 9, 11,// second triangle
      // face 3:
      12, 13, 14,// first triangle
      14, 13, 15,// second triangle
      // face 4:
      16, 17, 18,// first triangle
      18, 17, 19,// second triangle
      // face 5:
      20, 21, 22,// first triangle
      22, 21, 23,// second triangle
    };

    // fill with data
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 6 * 2 * 3, indexData, GL_STATIC_DRAW);

    mVertexArray.unbind();

    return true;
  }

  //: Render object.
  bool DCube3D::GUIRender(Canvas3D &c3d) const
  {
    (void)c3d;
#if 0
    //std::cerr << "DCube3D::Render(), Called. \n";

    GLColour(mColour);
    Vector<float,3> p1 = mDiag /2;
    Vector<float,3> p7 = -mDiag /2;

    Vector<float,3> p2 ( {p1[0], p7[1], p1[2]} );
    Vector<float,3> p3 ( {p7[0], p7[1], p1[2]} );
    Vector<float,3> p4 ( {p7[0], p1[1], p1[2]} );

    Vector<float,3> p5 ( {p7[0], p1[1], p7[2]} );
    Vector<float,3> p6 ( {p1[0], p1[1], p7[2]} );
    Vector<float,3> p8 ( {p1[0], p7[1], p7[2]} );


    glBegin(GL_QUADS);
    {
      // front
      glNormal3d(0,0,1);
      GLVertex(p1);
      GLVertex(p4);
      GLVertex(p3);
      GLVertex(p2);

      // top face
      glNormal3d(0,1,0) ;
      GLVertex(p1);
      GLVertex(p6);
      GLVertex(p5);
      GLVertex(p4);

       // back
      glNormal3d(0,0,-1) ;
      GLVertex(p8);
      GLVertex(p7);
      GLVertex(p5);
      GLVertex(p6);

      // bottom
      glNormal3d(0,-1,0) ;
      GLVertex(p2);
      GLVertex(p3);
      GLVertex(p7);
      GLVertex(p8);

      // left
      glNormal3d(-1,0,0) ;
      GLVertex(p3);
      GLVertex(p4);
      GLVertex(p5);
      GLVertex(p7);

      // right
      glNormal3d(1,0,0) ;
      GLVertex(p1);
      GLVertex(p2);
      GLVertex(p8);
      GLVertex(p6);
    }
    glEnd();
#else

    // check for errors
    {
      GLenum errCode = glGetError();
      if(errCode != GL_NO_ERROR) {
        SPDLOG_ERROR("Start Error: {} -> {} ", errCode, reinterpret_cast<const char *>(gluErrorString(errCode)));
      }
    }

    mShaderProgram->use();
    //mShaderProgram->setUniform("ViewProjection", c3d.viewProjection());

    // set the uniform
    glUniformMatrix4fv(mViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(c3d.projectionViewMatrix()));

    // bind the Vao
    mVertexArray.bind(0);

    // draw
    glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);

    // check for errors
    {
      GLenum errCode = glGetError();
      if(errCode != GL_NO_ERROR) {
        SPDLOG_ERROR("Error: {} -> {} ", errCode, reinterpret_cast<const char *>(gluErrorString(errCode)));
      }
    }

    mVertexArray.unbind();


#endif
    return true;
  }
}// namespace Ravl2
