//
// Created by charles on 24/08/24.
//

#include <nlohmann/json.hpp>

#include "Ravl2/IO/Save.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/Url.hh"

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

    auto parsedUrl = parseUrl(url);
    const auto &protocol = parsedUrl.mProtocol;
    const auto &rawFilename = parsedUrl.mFilename;
    const auto &ext = parsedUrl.mExtension;

    if(verbose) {
      SPDLOG_INFO("Opening output file: '{}' Protocol:'{}'  Extension:'{}'  Type:'{}'", url, protocol, ext, typeName(type));
    }

    // Create the context
    ProbeOutputContext ctx(url, rawFilename, protocol, ext, formatHint, type);
    ctx.mVerbose = verbose;
    return outputFormatMap().probe(ctx);
  }

}// namespace Ravl2