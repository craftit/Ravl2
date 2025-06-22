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

  namespace {
    std::unordered_map<std::type_index,std::string> &typeNameMap()
    {
      static std::unordered_map<std::type_index, std::string> map;
      return map;
    }
  }

  //! Register an alternative name for a type.
  bool registerTypeName(const std::type_info &type, const std::string &name)
  {
    std::unordered_map<std::type_index,std::string> &typeMap = typeNameMap();
    auto at = typeMap.find(type);
    if(at != typeMap.end()) {
      //std::("Type '{}' already has a name '{}' ,  requested '{}' ", demangle(type.name()), at->second, name);
      std::cerr << "Type '" << demangle(type.name()) << "' already has a name '" << at->second << "', requested '" << name << "'\n";
      return false;
    }
    typeMap[type] = name;
    return true;
  }

  //! Get a human-readable name for a type.
  std::string typeName(const std::type_index &type)
  {
    const auto &typeMap = typeNameMap();
    auto at = typeMap.find(type);
    if(at != typeMap.end()) {
      return at->second;
    }
    return demangle(type.name());
  }

  std::string typeName(const std::type_info &type)
  {
    return typeName(std::type_index(type));
  }

  const Eigen::IOFormat &defaultEigenFormat()
  {
    static Eigen::IOFormat ioFmt(Eigen::StreamPrecision,0," ","","[","]","(",")");
    return ioFmt;
  }

  namespace {
    [[maybe_unused]] bool reg1 = registerTypeName(typeid(std::string),"std::string");
    [[maybe_unused]] bool reg2 = registerTypeName(typeid(nlohmann::json),"nlohmann::json");
    [[maybe_unused]] bool reg3 = registerTypeName(typeid(std::vector<std::string>),"std::vector<std::string>");
  }

  void doNothing()
  {}

}// namespace Ravl2