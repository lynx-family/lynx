// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_VALUE_WRAPPER_VALUE_WRAPPER_UTILS_H_
#define CORE_VALUE_WRAPPER_VALUE_WRAPPER_UTILS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/base/lynx_export.h"
#include "core/public/pub_value.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace lepus {
class Value;
}
namespace runtime {
namespace js {
class JSIObjectWrapperManager;
}
}  // namespace runtime
namespace base {
namespace android {
class JavaValue;
}
}  // namespace base
namespace pub {

class ValueUtils {
 public:
  LYNX_EXPORT_FOR_DEVTOOL static lepus::Value ConvertValueToLepusValue(
      const Value& value,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0);
  static lepus::Value ConvertValueToLepusArray(
      const Value& value,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0);
  static lepus::Value ConvertValueToLepusTable(
      const Value& value,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0);
  static runtime::js::Value ConvertValueToPiperValue(
      runtime::js::Runtime& rt, const Value& value,
      runtime::js::JSIObjectWrapperManager* jsi_object_wrapper_manager =
          nullptr);
  static runtime::js::Value ConvertValueToPiperArray(runtime::js::Runtime& rt,
                                                     const Value& value);
  static runtime::js::Value ConvertValueToPiperObject(runtime::js::Runtime& rt,
                                                      const Value& value);

  static std::unique_ptr<uint8_t[]> ConvertPiperToArrayBuffer(
      runtime::js::Runtime& rt, const runtime::js::Object& o, size_t& length);
  static std::unique_ptr<Value> ConvertPiperArrayToPubValue(
      runtime::js::Runtime& rt, const runtime::js::Array& arr,
      const std::shared_ptr<PubValueFactory>& factory);
  static std::unique_ptr<Value> ConvertPiperObjectToPubValue(
      runtime::js::Runtime& rt, const runtime::js::Object& obj,
      const std::shared_ptr<PubValueFactory>& factory);

  // Some tricky logic for BigInt, such as { "id" : 8913891381287328398 }
  // will exist on js { "id" : { "__lynx_val__" : "8913891381287328398" }},
  // so { "__lynx_val__": } should be removed, and convert to  { "id" :
  // 8913891381287328398 }} again.
  // However, BigInt has a limited maximum value on some platforms. Currently,
  // Android use Int64, while iOS use string. So, a string is returned
  // for universality.
  // FIXME(wangying.666): if all platforms use a return value with same type in
  // the future, the code can be moved into ValueImplPiper.
  static bool IsBigInt(runtime::js::Runtime& rt,
                       const runtime::js::Object& obj);

  static bool ConvertBigIntToStringIfNecessary(runtime::js::Runtime& rt,
                                               const runtime::js::Object& obj,
                                               std::string& result);
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_VALUE_WRAPPER_VALUE_WRAPPER_UTILS_H_
