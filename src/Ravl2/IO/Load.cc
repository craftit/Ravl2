
#include "Load.hh"

namespace Ravl2
{

  [[nodiscard]] std::unique_ptr<StreamInputBase> openInput(const std::string &url, const std::type_info &type, std::string_view formatHint)
  {
    (void)url;
    (void)type;
    (void)formatHint;

    return nullptr;
  }

  [[nodiscard]] std::unique_ptr<StreamInputBase> openOutput(const std::string &url, const std::type_info &type, std::string_view formatHint)
  {
    (void)url;
    (void)type;
    (void)formatHint;

    return nullptr;
  }

}// namespace Ravl2