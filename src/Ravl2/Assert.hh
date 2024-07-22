
#pragma once

#include <spdlog/spdlog.h>

#ifndef RavlAssertMsg
#define RavlAssertMsg(cond, msg, args...) assert((cond) || SPDLOG_CRITICAL(msg, args))
#endif