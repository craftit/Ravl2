
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <spdlog/fmt/fmt.h>
#include <vector>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "Ravl2/Resource.hh"

namespace Ravl2
{
  struct ResourcePath {
    std::mutex m_access;
    std::map<std::string, std::vector<std::string>> paths;
  };

  bool loadEnvFile(std::string_view path)
  {
    // Check file exists
    if(!std::filesystem::exists(path)) {
      return false;
    }
    std::ifstream file(path.data());
    if(!file.is_open()) {
      SPDLOG_ERROR("Failed to open env file {}", path);
      return false;
    }
    SPDLOG_INFO("Loading env file '{}'", path);
    nlohmann::json config;
    file >> config;
    file.close();

    // Get the host name from the system
    std::array<char, 256> hostnameRaw {};
    gethostname(hostnameRaw.data(), hostnameRaw.size() - 1);
    std::string_view hostname(hostnameRaw.data());
    SPDLOG_TRACE("Hostname: '{}'", hostname);

    // Setup a lambda to read in the paths from a json object
    auto readPaths = [&](const nlohmann::json &obj) {
      // Go through each section type and and paths in the array for that section
      for(auto &section : obj.items()) {
        // Get the section name
        std::string sectionName = section.key();
        // Get the array of paths
        auto &paths = section.value();
        // Go through each path and add it
        for(auto &cpath : paths) {
          addResourcePath(sectionName, std::string(cpath));
        }
      }
    };

    // Is there a section for this host?
    if(config.contains(hostname)) {
      SPDLOG_TRACE("Found section for host '{}'", hostname);
      auto hostConfig = config[hostname.data()];
      readPaths(hostConfig["paths"]);
    }

    if(config.contains("default") && config["default"].contains("paths")) {
      readPaths(config["default"]["paths"]);
    }

    return true;
  }

  auto &resourcePaths()
  {
    static ResourcePath paths;
    static bool initialized = false;
    if(!initialized) {
      initialized = true;

      // Look in the users home directory for a .config/Reason directory
      auto envVar = std::getenv("HOME");
      if(envVar != nullptr) {
        loadEnvFile(fmt::format("{}/.config/Ravl2/env.json", envVar));
      }

      // First load any defined in the project config
      // This will silently fail if the file does not exist
      if(!loadEnvFile(fmt::format("{}/share/Ravl2/env.json", RAVL_SOURCE_DIR))) {
        // Not source directory, look in the install directory
        if(!loadEnvFile(fmt::format("{}/share/Ravl2/env.json", RAVL_INSTALL_PREFIX))) {
          // Look for the latest in the source directory
          addResourcePath("config", RAVL_INSTALL_PREFIX "/share/Ravl2");
          addResourcePath("data", RAVL_INSTALL_PREFIX "/data");
          addResourcePath("fonts", RAVL_INSTALL_PREFIX "/share/Ravl2/fonts");
          addResourcePath("models", RAVL_INSTALL_PREFIX "/share/Ravl2/models");
        }
      } else {
        // If we found the source directory, add the paths
        addResourcePath("config", RAVL_SOURCE_DIR "/share/Ravl2");
        addResourcePath("data", RAVL_SOURCE_DIR "/data");
        addResourcePath("fonts", RAVL_SOURCE_DIR "/share/Ravl2/fonts");
        addResourcePath("models", RAVL_SOURCE_DIR "/share/Ravl2/models");
      }
    }
    return paths;
  }

  void addResourcePath(const std::string_view &section, const std::string_view &path)
  {
    auto &paths = resourcePaths();
    std::lock_guard<std::mutex> lock(paths.m_access);
    // Get entry
    auto &entry = paths.paths[section.data()];
    // Check if path is already in the list
    for(auto &p : entry) {
      if(p == path)
        return;
    }
    // Add path
    entry.push_back(std::string(path));
  }

  std::string findFileResource(const std::string_view &section, const std::string_view &key, bool verbose)
  {
    if(key.empty())
      return "";
    // Is the key a full path?
    if(std::filesystem::is_regular_file(key)) {
      if(verbose) {
        SPDLOG_INFO("Key is a full path: {}", key);
      }
      return std::string(key);
    }
    // Start searching
    auto &paths = resourcePaths();
    std::lock_guard<std::mutex> lock(paths.m_access);
    // Get entry
    auto &entry = paths.paths[section.data()];
    if(entry.empty()) {
      SPDLOG_WARN("No paths for section '{}'", section);
    }
    // Go through paths and check if file exists on any of them
    for(auto &p : entry) {
      std::string path = fmt::format("{}/{}", p, key);
      if(verbose) {
        SPDLOG_INFO("Checking for file '{}'  Character: {} ", path, std::filesystem::is_regular_file(path));
      }
      if(std::filesystem::is_regular_file(path))
        return path;
    }
    // Not found
    return "";
  }

  std::string findDirectoryResource(const std::string_view &section, const std::string_view &key)
  {
    if(std::filesystem::is_directory(key))
      return std::string(key);
    // Start searching
    auto &paths = resourcePaths();
    std::lock_guard<std::mutex> lock(paths.m_access);
    // Get entry
    auto &entry = paths.paths[section.data()];
    // Go through paths and check if file exists on any of them
    for(auto &p : entry) {
      std::string path = fmt::format("{}/{}", p, key);
      if(std::filesystem::is_directory(path))
        return path;
    }
    // Not found
    return "";
  }

  std::string dumpResourcePaths()
  {
    std::string result;
    auto &paths = resourcePaths();
    std::lock_guard<std::mutex> lock(paths.m_access);
    for(auto &section : paths.paths) {
      result += fmt::format("Section: {}\n", section.first);
      for(auto &path : section.second) {
        result += fmt::format("  {}\n", path);
      }
    }
    return result;
  }

}// namespace Ravl2