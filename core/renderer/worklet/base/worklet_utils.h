// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_WORKLET_BASE_WORKLET_UTILS_H_
#define CORE_RENDERER_WORKLET_BASE_WORKLET_UTILS_H_

#include <string>

#include "core/public/pub_value.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace worklet {

class ValueConverter {
 public:
  static Napi::String ConvertStdStringToNapiString(Napi::Env env,
                                                   const std::string& value);
  static Napi::Boolean ConvertLepusBoolToNapiBoolean(Napi::Env env, bool value);
  static Napi::Number ConvertLepusInt32ToNapiNumber(Napi::Env env,
                                                    int32_t value);
  static Napi::Number ConvertLepusUInt32ToNapiNumber(Napi::Env env,
                                                     uint32_t value);
  static Napi::Number ConvertLepusInt64ToNapiNumber(Napi::Env env,
                                                    int64_t value);
  static Napi::Number ConvertLepusUInt64ToNapiNumber(Napi::Env env,
                                                     uint64_t value);
  static Napi::Number ConvertLepusNumberToNapiNumber(Napi::Env env,
                                                     const lepus::Value& value);
  static Napi::Array ConvertLepusValueToNapiArray(Napi::Env env,
                                                  const lepus::Value& value);
  static Napi::Object ConvertLepusValueToNapiObject(Napi::Env env,
                                                    const lepus::Value& value);
  static Napi::Value ConvertLepusValueToNapiValue(Napi::Env env,
                                                  const lepus::Value& value);

  static lepus::Value ConvertNapiValueToLepusValue(const Napi::Value& value);

  // TODO(chenyouhui):Maybe we can implement a Napi backend and support
  // conversions of any two Backend types
  static Napi::Object ConvertPubValueToNapiObject(Napi::Env env,
                                                  const pub::Value& value);

  static Napi::Value ConvertPubValueToNapiValue(Napi::Env env,
                                                const pub::Value& value);

  static Napi::Array ConvertPubValueToNapiArray(Napi::Env env,
                                                const pub::Value& value);
};

}  // namespace worklet
}  // namespace lynx

#endif  // CORE_RENDERER_WORKLET_BASE_WORKLET_UTILS_H_
