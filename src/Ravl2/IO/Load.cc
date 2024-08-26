
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/InputFormat.hh"

namespace Ravl2
{

  const nlohmann::json &defaultLoadFormatHint()
  {
    static nlohmann::json hint;
    return hint;
  }

  std::optional<StreamInputPlan> openInput(const std::string &url, const std::type_info &type, const nlohmann::json &formatHint)
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
    } else {
      protocol = "file";
      rawFilename = url;
    }

    // Get the extension
    auto extStart = url.find_last_of('.');
    std::string ext;
    if(extStart != std::string::npos) {
      ext = url.substr(extStart + 1);
    } else {
      ext = "";
    }

    if(verbose) {
      SPDLOG_INFO("Opening input stream for file: {} Extension:'{}' Protocol:'{}' ", url, ext, protocol);
    }

    ProbeInputContext ctx(url, rawFilename, protocol, ext, formatHint, type);

    // Do we want to be verbose?
    ctx.m_verbose = verbose;

    // Should we try and read data from the file into memory?
    if(protocol == "file") {
      std::unique_ptr<std::istream> tmpStrm = std::make_unique<std::ifstream>(rawFilename);
      if(!tmpStrm->good()) {
        return std::nullopt;
      }
      ctx.m_data = std::vector<uint8_t>(256);
      tmpStrm->read(reinterpret_cast<char *>(ctx.m_data.data()), std::streamsize(ctx.m_data.size()));
      auto numRead = tmpStrm->gcount();
      if(size_t(numRead) < ctx.m_data.size() && numRead >= 0) {
        ctx.m_data.resize(size_t(numRead));
      }
      if(verbose) {
        SPDLOG_INFO("Read {} bytes from file", ctx.m_data.size());
      }
      tmpStrm->seekg(0);
      ctx.mStream = std::move(tmpStrm);
    }

    return inputFormatMap().probe(ctx);
  }

}// namespace Ravl2