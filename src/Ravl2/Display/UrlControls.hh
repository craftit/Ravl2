//
// Parsing of the control part of display:// URLs, e.g. "display://img:Norm=Fixed=0,255:Mode=RingBuffer".
//

#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Ravl2/Display/Normalization.hh"

namespace Ravl2::DebugDisplay::UrlControls
{
  //! Case-insensitive string comparison.
  [[nodiscard]] inline bool iequals(std::string_view a, std::string_view b) noexcept
  {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
             return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
  }

  //! Split controls into the tokens between ':' separators, skipping empty ones.
  [[nodiscard]] inline std::vector<std::string_view> tokens(std::string_view controls)
  {
    std::vector<std::string_view> ret;
    size_t start = 0;
    while(start <= controls.size()) {
      const auto end = controls.find(':', start);
      const auto tok = controls.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
      if(!tok.empty())
        ret.push_back(tok);
      if(end == std::string_view::npos)
        break;
      start = end + 1;
    }
    return ret;
  }

  //! Strip any leading ':' and trailing '=' from a key, so ":Mode=" and "Mode" are the same key.
  [[nodiscard]] inline std::string_view bareKey(std::string_view key) noexcept
  {
    while(!key.empty() && key.front() == ':')
      key.remove_prefix(1);
    while(!key.empty() && key.back() == '=')
      key.remove_suffix(1);
    return key;
  }

  //! True if a token is exactly the flag (case-insensitive), e.g. "Clear" does not match "ClearOverlays".
  [[nodiscard]] inline bool hasFlag(std::string_view controls, std::string_view flag)
  {
    const auto name = bareKey(flag);
    for(auto tok : tokens(controls))
      if(iequals(tok, name))
        return true;
    return false;
  }

  //! Value of the first "key=value" token whose key matches (case-insensitive).
  [[nodiscard]] inline std::optional<std::string> value(std::string_view controls, std::string_view key)
  {
    const auto name = bareKey(key);
    for(auto tok : tokens(controls)) {
      const auto eq = tok.find('=');
      if(eq != std::string_view::npos && iequals(tok.substr(0, eq), name))
        return std::string(tok.substr(eq + 1));
    }
    return std::nullopt;
  }

  //! Parse "a,b" into two floats.
  [[nodiscard]] inline bool parsePair(std::string_view text, float &a, float &b)
  {
    const auto comma = text.find(',');
    if(comma == std::string_view::npos)
      return false;
    try {
      a = std::stof(std::string(text.substr(0, comma)));
      b = std::stof(std::string(text.substr(comma + 1)));
    } catch(...) {
      return false;
    }
    return true;
  }

  //! Parse normalisation controls:
  //!   Norm=Auto, Norm=Fixed=min,max, Norm=Pct:low,high (or Pct=low,high, Percentile:...).
  //! "Normalize" is accepted as well as "Norm". Names are case-insensitive.
  [[nodiscard]] inline std::optional<NormalizationSettings> parseNormalization(std::string_view controls)
  {
    const auto toks = tokens(controls);
    for(size_t i = 0; i < toks.size(); ++i) {
      const auto tok = toks[i];
      const auto eq = tok.find('=');
      if(eq == std::string_view::npos)
        continue;
      const auto key = tok.substr(0, eq);
      if(!iequals(key, "Norm") && !iequals(key, "Normalize"))
        continue;
      const auto val = tok.substr(eq + 1);
      const auto eq2 = val.find('=');
      const auto policy = val.substr(0, eq2);
      // Arguments follow '=' in the same token, or ':' as the next token (Pct:1,99).
      std::string_view args;
      if(eq2 != std::string_view::npos)
        args = val.substr(eq2 + 1);
      else if(i + 1 < toks.size() && toks[i + 1].find('=') == std::string_view::npos)
        args = toks[i + 1];

      NormalizationSettings ns {};
      if(iequals(policy, "Auto")) {
        ns.policy = NormalizationPolicy::Auto;
        return ns;
      }
      if(iequals(policy, "Fixed")) {
        ns.policy = NormalizationPolicy::Fixed;
        (void)parsePair(args, ns.minVal, ns.maxVal);
        return ns;
      }
      if(iequals(policy, "Pct") || iequals(policy, "Percentile")) {
        ns.policy = NormalizationPolicy::Percentile;
        (void)parsePair(args, ns.lowPct, ns.highPct);
        return ns;
      }
      return std::nullopt;
    }
    return std::nullopt;
  }
}// namespace Ravl2::DebugDisplay::UrlControls
