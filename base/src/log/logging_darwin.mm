// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging_darwin.h"
#include "base/include/log/alog_wrapper.h"

namespace lynx {
namespace base {
namespace logging {
namespace {

static alog_write_func_ptr alog_write_func_addr = nullptr;

alog_write_func_ptr GetAlogWriteFuncAddr() { return alog_write_func_addr; }

}  // namespace
void SetLynxLogMinLevel(int min_level) { SetMinLogLevel(min_level); }

void InternalLogNative(const char *file, int32_t line, int level, const char *message) {
  constexpr const char *kTag = "lynx";
  lynx::base::logging::PrintLogToLynxLogging(level, kTag, message);
}

void InitLynxLoggingNative(void *log_address, PlatformLogCallBack platform_log_channel,
                           bool print_logs_to_all_channels) {
  alog_write_func_addr = reinterpret_cast<alog_write_func_ptr>(log_address);
  lynx::base::logging::InitLynxLogging(GetAlogWriteFuncAddr, platform_log_channel,
                                       print_logs_to_all_channels);
}

}  // namespace logging
}  // namespace base
}  // namespace lynx
