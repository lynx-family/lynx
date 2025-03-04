// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxError.h"
#import "LynxSubErrorCode.h"

#include "core/shell/ios/lynx_runtime_facade_darwin.h"

namespace lynx {
namespace shell {

void NativeRuntimeFacadeDarwin::ReportError(const base::LynxError& error) {
  __strong LynxBackgroundRuntime* runtime = _runtime;
  NSMutableDictionary* customInfo = [NSMutableDictionary new];
  for (const auto& [key, value] : error.custom_info_) {
    [customInfo setValue:[NSString stringWithUTF8String:value.c_str()]
                  forKey:[NSString stringWithUTF8String:key.c_str()]];
  }
  std::string level_std_str =
      base::LynxError::GetLevelString(static_cast<int32_t>(error.error_level_));
  NSString* level = [NSString stringWithUTF8String:level_std_str.c_str()];
  LynxError* lynxError =
      [LynxError lynxErrorWithCode:error.error_code_
                           message:[NSString stringWithUTF8String:error.error_message_.c_str()]
                     fixSuggestion:[NSString stringWithUTF8String:error.fix_suggestion_.c_str()]
                             level:level
                        customInfo:customInfo
                      isLogBoxOnly:error.is_logbox_only_ ? YES : NO];
  [runtime onErrorOccurred:lynxError];
}

}  // namespace shell
}  // namespace lynx
