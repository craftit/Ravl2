//
// Created by charles on 24/08/24.
//

#include "Ravl2/IO/Save.hh"

namespace Ravl2
{

  [[nodiscard]] std::optional<OutputFormat::OutputPlanT> openOutput(const std::string &url, const std::type_info &type, const nlohmann::json &formatHint)
  {
    // Is there a protocol in the URL?
    auto protocolEnd = url.find("://");
    std::string protocol;
    std::string rawFilename;
    if(protocolEnd != std::string::npos)
    {
      protocol = url.substr(0, protocolEnd);
      rawFilename = url.substr(protocolEnd + 3);
    } else
    {
      protocol = "file";
      rawFilename = url;
    }

    // Get the extension
    auto extStart = url.find_last_of('.');
    std::string ext;
    if(extStart != std::string::npos)
    {
      ext = url.substr(extStart + 1);
    } else
    {
      ext = "";
    }

    // Create the context
    ProbeOutputContext ctx(url, rawFilename, protocol, ext, formatHint, type);

    return saveFormatMap().probe(ctx);

  }

}// namespace Ravl2