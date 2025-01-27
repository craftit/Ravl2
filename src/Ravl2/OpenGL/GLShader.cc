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

}// namespace Ravl2