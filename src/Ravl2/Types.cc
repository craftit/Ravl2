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

  //! Convert a time unit to a string
  [[nodiscard]]
  std::string_view toString(TimeUnitT const unit)
  {
    switch(unit) {
      case TimeUnitT::Hours: return "h";
      case TimeUnitT::Minutes: return "min";
      case TimeUnitT::Seconds: return "s";
      case TimeUnitT::Milliseconds: return "ms";
      case TimeUnitT::Microseconds: return "us";
      case TimeUnitT::Nanoseconds: return "ns";
      default: return "unknown";
    }
  }

  std::optional<TimeUnitT> fromString(std::string_view const unit)
  {
    if(unit == "h") return TimeUnitT::Hours;
    if(unit == "min") return TimeUnitT::Minutes;
    if(unit == "s") return TimeUnitT::Seconds;
    if(unit == "ms") return TimeUnitT::Milliseconds;
    if(unit == "us") return TimeUnitT::Microseconds;
    if(unit == "ns") return TimeUnitT::Nanoseconds;
    return std::nullopt;
  }

  //! Implements string-to-duration conversion for the specified template parameters
  template <typename Rep, typename Period>
  std::optional<std::chrono::duration<Rep, Period>> fromStringToDuration(const std::string& str)
  {
    using DurationType = std::chrono::duration<Rep, Period>;

    // Handle empty string case
    if (str.empty()) {
      return std::nullopt;
    }

    // First try to find where the number ends and the unit starts
    // Always parse as double to handle floating point correctly, even for integer types
    std::size_t pos = 0;
    double numericValue = 0.0;

    try {
      numericValue = std::stod(str, &pos);
    } catch (const std::exception&) {
      SPDLOG_ERROR("Failed to parse numeric part of duration: '{}'", str);
      return std::nullopt;
    }

    // If we couldn't parse any number or parsed the entire string as a number
    if (pos == 0) {
      return std::nullopt;
    } else if (pos == str.length()) {
      // If we parsed the entire string, it's just a number with no unit (treat as seconds)
      if constexpr (std::is_floating_point_v<Rep>) {
        return std::chrono::duration_cast<DurationType>(std::chrono::duration<double>(numericValue));
      } else {
        return std::chrono::duration_cast<DurationType>(std::chrono::duration<double>(numericValue));
      }
    }

    // Skip any whitespace between number and unit
    while (pos < str.length() && std::isspace(str[pos])) {
      ++pos;
    }

    // Extract the unit part
    std::string_view unitPart(str.data() + pos, str.length() - pos);

    if (unitPart.empty()) {
      return std::nullopt;
    }

    // Convert based on the unit
    auto timeUnit = fromString(unitPart);
    if (!timeUnit) {
      SPDLOG_ERROR("Unknown time unit '{}' in duration '{}'", unitPart, str);
      return std::nullopt;
    }

    // Convert to the target duration type
    switch (*timeUnit) {
      case TimeUnitT::Hours:
        return std::chrono::duration_cast<DurationType>(std::chrono::duration<double, std::ratio<3600>>(numericValue));
      case TimeUnitT::Minutes:
        return std::chrono::duration_cast<DurationType>(std::chrono::duration<double, std::ratio<60>>(numericValue));
      case TimeUnitT::Seconds:
        return std::chrono::duration_cast<DurationType>(std::chrono::duration<double>(numericValue));
      case TimeUnitT::Milliseconds:
        return std::chrono::duration_cast<DurationType>(std::chrono::duration<double, std::milli>(numericValue));
      case TimeUnitT::Microseconds:
        return std::chrono::duration_cast<DurationType>(std::chrono::duration<double, std::micro>(numericValue));
      case TimeUnitT::Nanoseconds:
        return std::chrono::duration_cast<DurationType>(std::chrono::duration<double, std::nano>(numericValue));
      default:
        SPDLOG_ERROR("Unhandled time unit in duration '{}'", str);
        return std::nullopt;
    }
  }

  //! Explicit instantiations for common duration types
  template std::optional<std::chrono::seconds> fromStringToDuration<int64_t, std::ratio<1>> (const std::string& str);
  template std::optional<std::chrono::duration<float>> fromStringToDuration<float, std::ratio<1>>(const std::string& str);
  template std::optional<std::chrono::duration<double>> fromStringToDuration<double, std::ratio<1>>(const std::string& str);
  template std::optional<std::chrono::milliseconds> fromStringToDuration<int64_t, std::milli>(const std::string& str);
  template std::optional<std::chrono::duration<float, std::milli>> fromStringToDuration<float, std::milli>(const std::string& str);
  template std::optional<std::chrono::duration<double, std::milli>> fromStringToDuration<double, std::milli>(const std::string& str);
  template std::optional<std::chrono::microseconds> fromStringToDuration<int64_t, std::micro>(const std::string& str);
  template std::optional<std::chrono::nanoseconds> fromStringToDuration<int64_t, std::nano>(const std::string& str);

  template std::optional<std::chrono::duration<int64_t, std::ratio<60>>> fromStringToDuration<int64_t, std::ratio<60>>(const std::string& str);
  template std::optional<std::chrono::duration<int64_t, std::ratio<3600>>> fromStringToDuration<int64_t, std::ratio<3600>>(const std::string& str);

  // Additional instantiations for test cases
  template std::optional<std::chrono::duration<double, std::ratio<3600>>> fromStringToDuration<double, std::ratio<3600>>(const std::string& str);
  template std::optional<std::chrono::duration<float, std::ratio<60>>> fromStringToDuration<float, std::ratio<60>>(const std::string& str);
  template std::optional<std::chrono::duration<double, std::micro>> fromStringToDuration<double, std::micro>(const std::string& str);

  void doNothing()
  {}

  namespace {
    [[maybe_unused]] bool reg1 = registerTypeName(typeid(std::string),"std::string");
    [[maybe_unused]] bool reg2 = registerTypeName(typeid(nlohmann::json),"nlohmann::json");
    [[maybe_unused]] bool reg3 = registerTypeName(typeid(std::vector<std::string>),"std::vector<std::string>");
    //! Implements string-to-duration conversion for the specified template parameters
  }


}// namespace Ravl2
