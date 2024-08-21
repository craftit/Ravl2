//
// Created by charles on 06/03/2021.
//

#pragma once
#include <vector>
#include <string>
#include <sstream>

namespace Ravl2
{
  //! Remove space characters from the beginning and end of the string.
  [[nodiscard]] std::string_view topAndTail(std::string_view word);

  //! Remove space characters from the beginning and end of the string.
  [[nodiscard]] std::string topAndTail(const std::string &word);

  //! top and tail all strings in a list.
  void topAndTailList(std::vector<std::string_view> &words);

  //! Split a string into sections delimited by 'd'
  [[nodiscard]] std::vector<std::string_view> split(std::string_view path, char d);

  //! Split a string into sections delimited by 'd'
  [[nodiscard]] std::vector<std::string> splitStrings(std::string path, char d);

  //! @brief Split a string into words
  //! @param str String to split
  //! @return Vector of words
  [[nodiscard]] std::vector<std::string> splitWords(const std::string &str);

  //! Generate a lower case version of a string.
  std::string toLowerCase(const std::string &str);

  //! Generate a unique identifier
  std::string generateUniqueId(size_t size);

  //! Generate an string indent
  inline std::string indent(int level)
  {
    return std::string(std::string::size_type(level), ' ');
  }

  template <typename T>
  inline std::string toString(const T &t)
  {
    std::stringstream ss;
    ss << t;
    return ss.str();
  }
}// namespace Ravl2