//
// Created by charles on 27/01/25.
//

#pragma once

#include <string>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GL/gl.h>
#include <GL/glext.h>


namespace Ravl2
{

  //! Shader object.

  class GLShader
  {
  public:
    //! Constructor from program text.
    GLShader(GLenum shaderType, std::string programText);

    //! Disable copy constructor.
    GLShader(const GLShader&) = delete;
    GLShader& operator=(const GLShader&) = delete;
    GLShader(GLShader&&) = delete;
    GLShader& operator=(GLShader&&) = delete;

    //! Destructor.
    ~GLShader();

    //! Compile the shader.
    //! Will throw an exception if the shader fails to compile.
    void compile();

    //! Get the shader object.
    [[nodiscard]] GLuint shader() const
    { return mShader; }

    //! Check if the shader has been compiled.
    [[nodiscard]] bool isCompiled() const
    { return mShader != 0; }
  private:
    GLenum mShaderType = 0;
    std::string mProgramText;
    GLuint mShader = 0;
  };

  //! Shader program object.

  class GLShaderProgram
  {
  public:
    GLShaderProgram() = default;

    //! Disable copy constructor.
    GLShaderProgram(const GLShaderProgram&) = delete;
    GLShaderProgram& operator=(const GLShaderProgram&) = delete;
    GLShaderProgram(GLShaderProgram&&) = delete;
    GLShaderProgram& operator=(GLShaderProgram&&) = delete;

    //! Constructor from vertex and fragment shaders.
    GLShaderProgram(const std::string &vertexShader,
                    const std::string &fragmentShader);

    //! Constructor from vertex and fragment shaders.
    GLShaderProgram(const std::shared_ptr<GLShader> &vertexShader,
                    const std::shared_ptr<GLShader> &fragmentShader);

    //! Destructor.
    ~GLShaderProgram();

    //! Link the program.
    //! Will throw an exception if the program fails to link.
    void link();

    //! Use the program.
    void use();

    //! Check if the program has been linked.
    [[nodiscard]] bool isLinked() const
    { return mProgram != 0; }

    //! Get uniform location.
    GLint getUniformLocation(const std::string &name);

  private:
    std::shared_ptr<GLShader> mVertexShader;
    std::shared_ptr<GLShader> mFragmentShader;
    GLuint mProgram = 0;
  };

  //! Vertex array object.

  class GLVertexArray
  {
  public:
    GLVertexArray();

    ~GLVertexArray();

    //! Bind the vertex array.
    void bind();

    //! Unbind the vertex array.
    void unbind();

    //! Check if the vertex array is bound.
    [[nodiscard]] bool isBound() const
    {
      return mVertexArray != 0;
    }

    //! Get the vertex array object.
    [[nodiscard]] GLuint vertexArray() const
    {
      return mVertexArray;
    }

  private:
    GLuint mVertexArray = 0;
  };

}// namespace Ravl2

