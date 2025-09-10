// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_SRC_BASE_TRACE_TRACE_HARMONY_H_
#define BASE_SRC_BASE_TRACE_TRACE_HARMONY_H_

#include <node_api.h>

namespace lynx {
namespace base {
namespace trace {

class LynxBaseTrace {
 public:
  static napi_value Init(napi_env env, napi_value exports);
  static napi_value NativeInitLynxBaseTrace(napi_env env,
                                            napi_callback_info info);
};

}  // namespace trace
}  // namespace base
}  // namespace lynx

#endif  // BASE_SRC_BASE_TRACE_TRACE_HARMONY_H_
