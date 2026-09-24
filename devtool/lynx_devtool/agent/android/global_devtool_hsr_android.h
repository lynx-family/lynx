// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_GLOBAL_DEVTOOL_HSR_ANDROID_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_GLOBAL_DEVTOOL_HSR_ANDROID_H_

#include <cstdint>
#include <string>

namespace lynx {
namespace devtool {
// The resource provider retains an ID, never a native request pointer.
bool FetchHSRScriptSource(const std::string& url, int64_t request_id);
void CompleteHSRScriptSource(int64_t request_id, std::string source,
                             std::string error);
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_GLOBAL_DEVTOOL_HSR_ANDROID_H_
