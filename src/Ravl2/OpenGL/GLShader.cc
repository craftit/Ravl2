//
// Created by charles on 27/01/25.
//

#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>
#include "Ravl2/OpenGL/GLShader.hh"

namespace Ravl2
{

  GLShader::GLShader(GLenum shaderType, std::string programText)
   : mShaderType(shaderType),
    mProgramText(programText)
  {}

  GLShader::~GLShader()
  {
    // TODO: Check if we're in the right context.
    if(mShader != 0) {
      glDeleteShader(mShader);
    }
  }

  void GLShader::compile()
  {
    // Create a shader object
    mShader = glCreateShader(mShaderType);
    if(mShader == 0) {
      SPDLOG_ERROR("Error creating shader object");
      throw std::runtime_error("Error creating shader object");
    }

    // Load the shader source
    const char *c_str = mProgramText.c_str();
    glShaderSource(mShader, 1, &c_str, nullptr);

    // Compile the shader
    glCompileShader(mShader);

    // Check the compile status
    GLint status;
    glGetShaderiv(mShader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
      GLint infoLogLength = 0;
      glGetShaderiv(mShader, GL_INFO_LOG_LENGTH, &infoLogLength);
      if(infoLogLength <= 0) {
        throw std::runtime_error("Error compiling shader: no info log");
      }
      std::vector<GLchar> infoLog((size_t(infoLogLength)));
      glGetShaderInfoLog(mShader, infoLogLength, nullptr, infoLog.data());
      SPDLOG_ERROR("Error compiling shader: {}", infoLog.data());
      throw std::runtime_error("Error compiling shader: ");
    }
  }

  // ------------------------------------------------------------

  GLShaderProgram::GLShaderProgram(const std::string &vertexShader,
                                  const std::string &fragmentShader)
   : mVertexShader(std::make_shared<GLShader>(GL_VERTEX_SHADER, vertexShader)),
     mFragmentShader(std::make_shared<GLShader>(GL_FRAGMENT_SHADER, fragmentShader))
  {}


  GLShaderProgram::GLShaderProgram(const std::shared_ptr<GLShader> &vertexShader,
                  const std::shared_ptr<GLShader> &fragmentShader)
   : mVertexShader(vertexShader),
     mFragmentShader(fragmentShader)
  {}

  GLShaderProgram::~GLShaderProgram()
  {
    if(mProgram != 0) {
      if(mVertexShader != nullptr) {
        glDetachShader(mProgram, mVertexShader->shader());
      }
      if(mFragmentShader != nullptr) {
        glDetachShader(mProgram, mFragmentShader->shader());
      }
      glDeleteProgram(mProgram);
    }
  }

  void GLShaderProgram::link()
  {
    // Create the program object
    mProgram = glCreateProgram();
    if(mProgram == 0) {
      SPDLOG_ERROR("Error creating program object");
      throw std::runtime_error("Error creating program object");
    }

    if(!mVertexShader->isCompiled()) {
      mVertexShader->compile();
    }

    if(!mFragmentShader->isCompiled()) {
      mFragmentShader->compile();
    }

    // Attach the shaders
    glAttachShader(mProgram, mVertexShader->shader());
    glAttachShader(mProgram, mFragmentShader->shader());

    // Link the program
    glLinkProgram(mProgram);

    // Check the link status
    GLint status;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
      GLint infoLogLength = 0;
      glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &infoLogLength);
      if(infoLogLength <= 0) {
        throw std::runtime_error("Error linking program: no info log");
      }
      std::vector<GLchar> infoLog((size_t(infoLogLength)));
      glGetProgramInfoLog(mProgram, infoLogLength, nullptr, infoLog.data());
      SPDLOG_ERROR("Error linking program: {}", infoLog.data());
      throw std::runtime_error("Error linking program: ");
    }

  }

  //! Use the program.
  void GLShaderProgram::use()
  {
    if(mProgram == 0) {
      link();
    }
    assert(mProgram != 0);
    glUseProgram(mProgram);
  }

  GLint GLShaderProgram::getUniformLocation(const std::string &name)
  {
    assert(isLinked());
    return glGetUniformLocation(mProgram, name.c_str());
  }

  // ------------------------------------------------------------

  GLVertexBuffer::~GLVertexBuffer()
  {
    if(!mBuffers.empty()) {
      glDeleteBuffers(GLsizei(mBuffers.size()), mBuffers.data());
    }
  }

  void GLVertexBuffer::generate(size_t size)
  {
    assert(mBuffers.empty());
    mBuffers = std::vector<GLuint>(size, 0);
    glGenBuffers(GLsizei(size), mBuffers.data());
  }

  void GLVertexBuffer::bind(GLenum target, size_t ind) const
  {
    assert(!mBuffers.empty());
    glBindBuffer(target, mBuffers.at(ind));
  }

  void GLVertexBuffer::unbind() const
  {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  // ------------------------------------------------------------

  GLVertexArray::~GLVertexArray()
  {
    if(!mVertexArray.empty()) {
      glDeleteVertexArrays(int(mVertexArray.size()), mVertexArray.data());
    }
    mVertexArray.clear();
  }

  //! Create the vertex array.
  void GLVertexArray::generate(size_t size)
  {
    assert(mVertexArray.empty());
    mVertexArray = std::vector<GLuint>(size, 0);
    glGenVertexArrays(size, mVertexArray.data());
  }

  //! Bind the vertex array.
  void GLVertexArray::bind(size_t ind) const
  {
    assert(!mVertexArray.empty());
    glBindVertexArray(mVertexArray.at(ind));
  }

  //! Unbind the vertex array.
  void GLVertexArray::unbind() const
  {
    glBindVertexArray(0);
  }

  //! Add a buffer to the vertex array.
  void GLVertexArray::addBuffer(const std::shared_ptr<GLVertexBuffer> &buffer)
  {
    // Check we have a valid buffer
    assert(buffer != nullptr);

    // Check it's not already in the list
    if(std::find(mBuffers.begin(), mBuffers.end(), buffer) != mBuffers.end()) {
      return;
    }
    // Add buffer to the list used by the vertex array
    mBuffers.push_back(buffer);
  }


  //! Add data from a buffer to the vertex array.
  void GLVertexArray::addBuffer(const std::shared_ptr<GLVertexBuffer> &buffer,
                                 GLuint index, GLint size, GLenum type,
                                 GLboolean normalized, GLsizei stride, const void *pointer)
  {
    assert(buffer != nullptr);
    assert(!mVertexArray.empty());

    // Add buffer to the list used by the vertex array
    addBuffer(buffer);

    glEnableVertexAttribArray(index);

    // Set up the vertex attributes
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
  }




}// namespace Ravl2