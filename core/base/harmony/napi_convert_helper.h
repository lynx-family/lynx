// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_HARMONY_NAPI_CONVERT_HELPER_H_
#define CORE_BASE_HARMONY_NAPI_CONVERT_HELPER_H_

#include <node_api.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace base {

class NapiConvertHelper {
 public:
  static lepus_value JSONToLepusValue(napi_env env, napi_value obj);

  static lepus_value ConvertToLepusValue(napi_env env, napi_value obj);

  static void AssembleArray(napi_env env, napi_value array, uint32_t index,
                            const lepus::Value& value);

  static void AssembleMap(napi_env env, napi_value object, const char* key,
                          const lepus::Value& value);

  static napi_value CreateNapiValue(napi_env env, const lepus::Value& value);

  static napi_value CloneNapiValue(napi_env env, napi_value obj);
};
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_HARMONY_NAPI_CONVERT_HELPER_H_
