
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/InputFormat.hh"

namespace Ravl2
{
  const nlohmann::json &defaultFormatHint()
  {
    static nlohmann::json hint;
    return hint;
  }

  [[nodiscard]] std::optional<StreamInputPlan> openInput(const std::string &url, const std::type_info &type, std::string_view formatHint)
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

    ProbeInputContext ctx(url, rawFilename, protocol, ext, formatHint, type);

    // Open the file
    std::unique_ptr<std::istream> tmpStrm =  std::make_unique<std::ifstream>(rawFilename);
    if(!tmpStrm->good())
    {
      return std::nullopt;
    }
    ctx.m_data = std::vector<uint8_t>(256);
    tmpStrm->read(reinterpret_cast<char *>(ctx.m_data.data()), std::streamsize(ctx.m_data.size()));
    auto numRead = tmpStrm->gcount();
    if(size_t(numRead) < ctx.m_data.size() && numRead >= 0)
    {
      ctx.m_data.resize(size_t(numRead));
    }
    tmpStrm->seekg(0);
    ctx.mStream = std::move(tmpStrm);

    return InputFormatMap().probe(ctx);
  }


}// namespace Ravl2