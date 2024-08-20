
#include "Ravl2/FileIO.hh"

namespace Ravl2
{

  [[nodiscard]] std::unique_ptr<StreamBase> openInput(const std::string &url, const std::type_info &type,std::string_view formatHint)
  {
    (void)url;
    (void)type;
    (void)formatHint;

    return nullptr;
  }

  [[nodiscard]] std::unique_ptr<StreamBase> openOutput(const std::string &url, const std::type_info &type,std::string_view formatHint)
  {
    (void)url;
    (void)type;
    (void)formatHint;

    return nullptr;
  }

}