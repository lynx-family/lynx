// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MEMORY_USAGE_QUERY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MEMORY_USAGE_QUERY_H_

#include <napi/native_api.h>

namespace lynx::harmony {

void InitMemoryUsageQuery(napi_env env, napi_value exports);

}  // namespace lynx::harmony

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MEMORY_USAGE_QUERY_H_
