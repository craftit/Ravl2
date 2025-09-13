
#include "Ravl2/Logging.hh"

namespace Ravl2
{

  void initializeLogging()
  {
#ifdef NDEBUG
    // Release mode: set to info level by default
    spdlog::set_level(spdlog::level::info);
#else
    // Debug mode: set to debug level by default
    spdlog::set_level(spdlog::level::debug);
#endif
  }

  void initializeLogging(spdlog::level::level_enum overrideLevel)
  {
    // If the override level is specified, use it
    spdlog::set_level(overrideLevel);
  }

}// namespace Ravl2
