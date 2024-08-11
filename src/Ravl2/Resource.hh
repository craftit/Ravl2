
#pragma once

#include <string>

namespace Ravl2
{
  //! Add a new resource path, to be searched for files. Duplicate paths are ignored.
  //! @param path Path to add
  void addResourcePath(const std::string_view &section, const std::string_view &path);

  //! Find a file resource
  //! @param section Section name.  'config', 'data', 'models'
  //! @param key Key name
  //! @return File path, it will be empty if not found
  std::string findFileResource(const std::string_view &section, const std::string_view &key, bool verbose = false);

  //! Find a directory resource
  //! @param section Section name
  //! @param key Key name, maybe empty
  //! @return Directory path, it will be empty if not found
  std::string findDirectoryResource(const std::string_view &section, const std::string_view &key = "");

  //! Dump the resource paths to a string
  //! @return String containing the resource paths
  std::string dumpResourcePaths();

}// namespace Ravl2