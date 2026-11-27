//
// URL parsing for RavlIO.
//

#pragma once

#include <string>
#include <string_view>

namespace Ravl2
{
  //! Parsed URL fields used by input and output format selection.
  struct ParsedUrl {
    std::string mProtocol;
    std::string mFilename;
    std::string mExtension;
  };

  //! Parse a RavlIO URL into protocol, filename, and normalized extension.
  //! URLs without a scheme use the "file" protocol. Extensions are lowercase
  //! and omit the leading dot.
  [[nodiscard]] ParsedUrl parseUrl(std::string_view url);
}// namespace Ravl2