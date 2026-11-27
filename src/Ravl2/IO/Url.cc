//
// URL parsing for RavlIO.
//

#include <filesystem>

#include "Ravl2/StringUtils.hh"
#include "Ravl2/IO/Url.hh"

namespace Ravl2
{
  ParsedUrl parseUrl(std::string_view url)
  {
    ParsedUrl parsed;
    const auto protocolEnd = url.find("://");
    if(protocolEnd == std::string_view::npos) {
      parsed.mProtocol = "file";
      parsed.mFilename = url;
    } else {
      parsed.mProtocol = url.substr(0, protocolEnd);
      parsed.mFilename = url.substr(protocolEnd + 3);
    }

    auto extension = std::filesystem::path(parsed.mFilename).extension().string();
    if(!extension.empty() && extension.front() == '.')
      extension.erase(extension.begin());
    parsed.mExtension = toLowerCase(extension);
    return parsed;
  }
}// namespace Ravl2