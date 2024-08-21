//
// Created by charles on 06/03/2021.
//

#include <algorithm>
#include <random>
#include "Ravl2/StringUtils.hh"

namespace Ravl2
{
  std::string_view topAndTail(std::string_view word)
  {
    while(!word.empty() && ::isspace(word.front()))
      word.remove_prefix(1);
    while(!word.empty() && ::isspace(word.back()))
      word.remove_suffix(1);
    return word;
  }

  //! Remove space characters from the beginning and end of the string.
  std::string topAndTail(const std::string &word)
  {
    return std::string(topAndTail(std::string_view(word)));
  }

  void topAndTailList(std::vector<std::string_view> &words)
  {
    for(auto &s : words)
      s = topAndTail(s);
  }

  std::vector<std::string_view> split(std::string_view path, char d)
  {
    std::vector<std::string_view> r;
    std::string_view::size_type j = 0;
    for(std::string_view::size_type i = 0; i < path.length(); i++) {
      if(path[i] == d) {
        auto cur = path.substr(j, i - j);
        if(cur.length()) {
          r.push_back(cur);
        }
        j = i + 1;
      }
    }
    if(j < path.length()) {
      r.push_back(path.substr(j));
    }
    return r;
  }

  //! Split a string into sections delimited by 'd'
  [[nodiscard]] std::vector<std::string> splitStrings(std::string path, char d)
  {
    auto r = split(std::string_view(path), d);
    std::vector<std::string> result;
    result.reserve(r.size());
    for(auto &s : r) {
      result.push_back(std::string(s));
    }
    return result;
  }

  [[nodiscard]] std::vector<std::string> splitWords(const std::string &str)
  {
    std::vector<std::string> words;
    std::string word;
    for(auto c : str) {
      if(std::isspace(c)) {
        if(!word.empty()) {
          words.push_back(word);
          word.clear();
        }
      } else {
        word.push_back(c);
      }
    }
    if(!word.empty()) {
      words.push_back(word);
    }
    return words;
  }

  std::string toLowerCase(const std::string &str)
  {
    std::string result;
    result.resize(str.size());
    std::transform(str.begin(), str.end(), result.begin(), ::tolower);
    return result;
  }

  std::mt19937 g_randomNumberGenerator;

  std::string generateUniqueId(size_t size)
  {
    std::uniform_int_distribution<int8_t> distributeInt('A', 'Z');

    std::string ret;
    ret.reserve(size);
    for(size_t i = 0; i < size; i++) {
      ret += distributeInt(g_randomNumberGenerator);
    }
    return ret;
  }

}// namespace Ravl2