//
// Created by charles on 27/01/25.
//

#pragma once

#include <string>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GL/gl.h>
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"


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

  //! Vertex buffer object.
  //! The main goal of this class to manage the OpenGL objects life cycle.

  class GLVertexBuffer
  {
  public:
    GLVertexBuffer() = default;

    //! Disable copy constructor.
    GLVertexBuffer(const GLVertexBuffer&) = delete;
    GLVertexBuffer& operator=(const GLVertexBuffer&) = delete;
    GLVertexBuffer(GLVertexBuffer&&) = delete;
    GLVertexBuffer& operator=(GLVertexBuffer&&) = delete;

    //! Destructor.
    ~GLVertexBuffer();

    //! Create the buffer.
    void generate(size_t size = 1);

    //! Bind the buffer.
    void bind(GLenum target, size_t ind = 0) const;

    //! Unbind the buffer.
    void unbind() const;

    //! Get the buffer object.
    [[nodiscard]] GLuint buffer(size_t ind = 0) const
    {
      return mBuffers.at(ind);
    }

    //! Get the number of buffers.
    [[nodiscard]] size_t size() const
    {
      return mBuffers.size();
    }

  private:
    std::vector<GLuint> mBuffers;
  };


  //! Vertex array object.
  //! The main goal of this class to manage the OpenGL objects life cycle and dependencies.

  class GLVertexArray
  {
  public:
    GLVertexArray() = default;

    //! Disable copy constructor.
    GLVertexArray(const GLVertexArray&) = delete;
    GLVertexArray& operator=(const GLVertexArray&) = delete;
    GLVertexArray(GLVertexArray&&) = delete;
    GLVertexArray& operator=(GLVertexArray&&) = delete;

    //! Destructor.
    ~GLVertexArray();

    //! Create the vertex array.
    void generate(size_t size = 1);

    //! Bind the vertex array.
    void bind(size_t ind) const;

    //! Unbind the vertex array.
    void unbind() const;

    //! Get the vertex array object.
    [[nodiscard]] GLuint vertexArray(size_t ind) const
    {
      return mVertexArray.at(ind);
    }

    //! Add data from a buffer to the vertex array.
    void addBuffer(const std::shared_ptr<GLVertexBuffer> &buffer,
                       GLuint index, GLint size, GLenum type,
                       GLboolean normalized, GLsizei stride, const void *pointer);

    //! Add a buffer to the vertex array.
    void addBuffer(const std::shared_ptr<GLVertexBuffer> &buffer);

    //! Get the number of arrays.
    [[nodiscard]] size_t size() const
    {
      return mVertexArray.size();
    }

  private:
    std::vector<GLuint> mVertexArray;
    std::vector<std::shared_ptr<GLVertexBuffer>> mBuffers;
  };


  //! Texture object.
  //! The main goal of this class to manage the OpenGL objects life cycle.

  class GLTexture
  {
  public:
    GLTexture() = default;

    //! Disable copy constructor.
    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;
    GLTexture(GLTexture&&) = delete;
    GLTexture& operator=(GLTexture&&) = delete;

    //! Destructor.
    ~GLTexture();

    //! Create the texture.
    void generate(size_t size = 1);

    //! Bind the texture.
    void bind(GLenum target, size_t ind = 0) const;

    //! Set the texture data.
    void setData(size_t ind,
                  GLenum target, GLint level, GLint internalFormat,
                     GLsizei width, GLsizei height, GLint border,
                     GLenum format, GLenum type, const void *data);

    //! Set texture data from a grey scale array.
    void setArray(size_t ind,const Array<uint8_t,2> &data);

    //! Set texture data from a colour array.
    void setArray(size_t ind,const Array<PixelBGR8,2> &data);

    //! Set texture data from a colour + alpha array.
    void setArray(size_t ind,const Array<PixelBGRA8,2> &data);

    //! Get the texture object.
    [[nodiscard]] GLuint texture(size_t ind = 0) const
    {
      return mTextures.at(ind);
    }

    //! Get the number of textures.
    [[nodiscard]] size_t size() const
    {
      return mTextures.size();
    }

  private:
    std::vector<GLuint> mTextures;
  };


}// namespace Ravl2

