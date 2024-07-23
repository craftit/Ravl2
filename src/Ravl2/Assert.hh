
#pragma once

#include <spdlog/spdlog.h>

#define RavlAlwaysAssert(cond) { if(cond) { SPDLOG_CRITICAL("Assert failed: " #cond ); abort(); }}
#define RavlAlwaysAssertMsg(cond, ...) { if(cond) { SPDLOG_CRITICAL(__VA_ARGS__); abort(); }}

#ifdef NDEBUG
#define RavlAssertMsg(cond, ...) {}
#define RavlAssert(cond) {}
#else
#define RavlAssert(cond) { if(cond) { SPDLOG_CRITICAL("Assert failed: " #cond ); abort(); }}
#ifndef RavlAssertMsg
#define RavlAssertMsg(cond, ...) { if(cond) { SPDLOG_CRITICAL(__VA_ARGS__); abort(); }}
#endif
#endif