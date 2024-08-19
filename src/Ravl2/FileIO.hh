
#pragma once

#include <string>

namespace Ravl2
{
  //! @brief Load a file into an object.
  //! @details This function will load a file into an object.
  //! The file is loaded using the cereal library.
  //! @param url - The filename to load.
  //! @param object - The object to load into.
  //! @return True if the file was loaded successfully.

  template <typename ObjectT>
  bool load(const std::string &url, ObjectT &object)
  {
    return false;
  }

  //! @brief Save an object to a file.


}