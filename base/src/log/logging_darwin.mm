// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging_darwin.h"
#import <LynxBase/LynxBaseService.h>             // nogncheck
#import <LynxBase/LynxBaseServiceLogProtocol.h>  // nogncheck
#include "base/include/log/alog_wrapper.h"

namespace lynx {
namespace base {
namespace logging {
namespace {

alog_write_func_ptr GetAlogWriteFuncAddr() {
  id service = LynxBaseService(LynxBaseServiceLogProtocol);
  if (service) {
    return (alog_write_func_ptr)[service getWriteFunction];
  }

  return nullptr;
}

}  // namespace
void SetLynxLogMinLevel(int min_level) { SetMinLogLevel(min_level); }

void InternalLogNative(const char *file, int32_t line, int level, const char *message) {
  constexpr const char *kTag = "lynx";
  lynx::base::logging::PrintLogToLynxLogging(level, kTag, message);
}

void InitLynxLoggingNative(bool print_logs_to_all_channels) {
  lynx::base::logging::InitLynxLogging(GetAlogWriteFuncAddr, PrintLogMessageByLogDelegate,
                                       print_logs_to_all_channels);
}

}  // namespace logging
}  // namespace base
}  // namespace lynx
