// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_VALUE_WRAPPER_NAPI_VALUE_IMPL_OPAQUE_NAPI_PRIMJS_H_
#define CORE_VALUE_WRAPPER_NAPI_VALUE_IMPL_OPAQUE_NAPI_PRIMJS_H_

#include <cstddef>
#include <memory>
#include <string>

#include "core/public/pub_value.h"

namespace lynx {
namespace pub {
class ValueUtilsOpaqueNapiPrimJS {
 public:
  static void* ConvertPubValueToOpaqueNapiValue(void* opaque_env,
                                                const Value& value);
  static std::unique_ptr<Value> CreateValueWithOpaqueNapiArgs(
      void* opaque_env, void* opaque_value);
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_VALUE_WRAPPER_NAPI_VALUE_IMPL_OPAQUE_NAPI_PRIMJS_H_
