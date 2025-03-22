//
// Created by charles on 05/08/24.
//
#include "Ravl2/Types.hh"
#include "nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <cxxabi.h>

namespace Ravl2
{
  //  static_assert(sizeof(Vector2f) == 2*sizeof(float),"Vector2f is not packed");
  //  static_assert(sizeof(Vector3f) == 3*sizeof(float),"Vector2f is not packed");

  std::string toString(Vector3d v)
  {
    return fmt::format("({},{},{})", v[0], v[1], v[2]);
  }

  std::string toString(Vector3f v)
  {
    return fmt::format("({},{},{})", v[0], v[1], v[2]);
  }

  std::string toString(Vector2f v)
  {
    return fmt::format("({},{})", v[0], v[1]);
  }

  std::string toString(Vector2d v)
  {
    return fmt::format("({},{})", v[0], v[1]);
  }

  //! Demangle a C++ name

  std::string demangle(const char *name)
  {
    std::string demangled;
    int status = 1;
    char *demangled_ptr = nullptr;
    demangled_ptr = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if(status != 0 || demangled_ptr == nullptr) {
      SPDLOG_ERROR("Failed to demangle name '{}' ", name);
      demangled = name;
    } else {
      demangled = demangled_ptr;
    }
    free(demangled_ptr);
    return demangled;
  }

  std::string typeName(const std::type_info &type)
  {
    if(type == typeid(nlohmann::json)) {
      return "nlohmann::json";
    }
    if(type == typeid(std::string)) {
      return "std::string";
    }
    return demangle(type.name());
  }

  //! Get a human-readable name for a type.
  std::string typeName(const std::type_index &type)
  {
    if(type == typeid(nlohmann::json)) {
      return "nlohmann::json";
    }
    if(type == typeid(std::string)) {
      return "std::string";
    }
    return demangle(type.name());
  }

  const Eigen::IOFormat &defaultEigenFormat()
  {
    static Eigen::IOFormat ioFmt(Eigen::StreamPrecision,0," ","","[","]","(",")");
    return ioFmt;
  }


  void doNothing()
  {}

}// namespace Ravl2