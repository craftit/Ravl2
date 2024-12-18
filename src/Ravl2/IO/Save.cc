//
// Created by charles on 24/08/24.
//

#include "Ravl2/IO/Save.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include <nlohmann/json.hpp>

namespace Ravl2
{
  const nlohmann::json &defaultSaveFormatHint(bool verbose)
  {
    static nlohmann::json hint;
    static nlohmann::json hint2 = {{"verbose", true}};
    if(verbose) {
      return hint2;
    }
    return hint;
  }

  [[nodiscard]] std::optional<StreamOutputPlan> openOutput(const std::string &url, const std::type_info &type, const nlohmann::json &formatHint)
  {
    bool verbose = false;
    if(formatHint.is_object()) {
      verbose = formatHint.value("verbose", verbose);
    }

    // Is there a protocol in the URL?
    auto protocolEnd = url.find("://");
    std::string protocol;
    std::string rawFilename;
    if(protocolEnd != std::string::npos) {
      protocol = url.substr(0, protocolEnd);
      rawFilename = url.substr(protocolEnd + 3);

#if defined(__unix__)
      if(protocol == "display") {
        protocol = "dlib";
      }
#else
      if(protocol == "display") {
        protocol = "opencv";
      }
#endif


    } else {
      protocol = "file";
      rawFilename = url;
    }

    // Get the extension
    auto extStart = rawFilename.find_last_of('.');
    std::string ext;
    if(extStart != std::string::npos) {
      ext = rawFilename.substr(extStart + 1);
    } else {
      ext = "";
    }
    if(verbose) {
      SPDLOG_INFO("Opening output file: '{}' Protocol:'{}'  Extension:'{}'  Type:'{}'", url, protocol, ext, typeName(type));
    }

    // Create the context
    ProbeOutputContext ctx(url, rawFilename, protocol, ext, formatHint, type);
    ctx.m_verbose = verbose;
    return outputFormatMap().probe(ctx);
  }

}// namespace Ravl2