
#pragma once

#include <spdlog/spdlog.h>

namespace Ravl2
{
  //! Helper to change the logging level for a scope
  class SetSPDLogLevel
  {
  public:
    explicit SetSPDLogLevel(spdlog::level::level_enum level)
    {
      m_oldLevel = spdlog::get_level();
      spdlog::set_level(level);
    }

    ~SetSPDLogLevel()
    {
      spdlog::set_level(m_oldLevel);
    }

  private:
    spdlog::level::level_enum m_oldLevel;
  };
}// namespace Ravl2

#define RavlAlwaysAssert(cond)                  \
  {                                             \
    if(!(cond)) {                               \
      SPDLOG_CRITICAL("Assert failed: " #cond); \
      abort();                                  \
    }                                           \
  }

#define RavlAlwaysAssertMsg(cond, ...) \
  {                                    \
    if(!(cond)) {                      \
      SPDLOG_CRITICAL(__VA_ARGS__);    \
      abort();                         \
    }                                  \
  }

#ifdef NDEBUG
#define RavlAssertMsg(cond, ...) \
  {}
#define RavlAssert(cond) \
  {}
#else
#define RavlAssert(cond)                        \
  {                                             \
    if(!(cond)) {                               \
      SPDLOG_CRITICAL("Assert failed: " #cond); \
      abort();                                  \
    }                                           \
  }
#ifndef RavlAssertMsg
#define RavlAssertMsg(cond, ...)    \
  {                                 \
    if(!(cond)) {                   \
      SPDLOG_CRITICAL(__VA_ARGS__); \
      abort();                      \
    }                               \
  }
#endif
#endif