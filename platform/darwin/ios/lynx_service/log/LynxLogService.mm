// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLogService.h"
#import <Foundation/Foundation.h>

namespace lynx {
namespace base {
namespace logging {

[[maybe_unused]] void logWrite(unsigned int level, const char *tag, const char *format) {
  if (format == NULL) {
    return;
  }
  NSLog(@"[%s] %s", tag == NULL ? "" : tag, format);
}
}  // namespace logging
}  // namespace base
}  // namespace lynx

@LynxServiceRegister(LynxLogService) @implementation LynxLogService

+ (LynxServiceScope)serviceScope {
  return LynxServiceScopeDefault;
}

+ (NSUInteger)serviceType {
  return kLynxServiceLog;
}

+ (NSString *)serviceBizID {
  return DEFAULT_LYNX_SERVICE;
}

- (void *)getWriteFunction {
  return (void *)lynx::base::logging::logWrite;
}

@end
