// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_VALUE_WRAPPER_VALUE_WRAPPER_UTILS_H_
#define CORE_VALUE_WRAPPER_VALUE_WRAPPER_UTILS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/public/pub_value.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace lepus {
class Value;
}
namespace base {
namespace android {
class JavaValue;
}
}  // namespace base
namespace pub {

class ValueUtils {
 public:
  static lepus::Value ConvertValueToLepusValue(
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
  static piper::Value ConvertValueToPiperValue(piper::Runtime& rt,
                                               const Value& value);
  static piper::Value ConvertValueToPiperArray(piper::Runtime& rt,
                                               const Value& value);
  static piper::Value ConvertValueToPiperObject(piper::Runtime& rt,
                                                const Value& value);

  static std::unique_ptr<uint8_t[]> ConvertPiperToArrayBuffer(
      piper::Runtime& rt, const piper::Object& o, size_t& length);
  static std::unique_ptr<Value> ConvertPiperArrayToPubValue(
      piper::Runtime& rt, const piper::Array& arr,
      const std::shared_ptr<PubValueFactory>& factory);
  static std::unique_ptr<Value> ConvertPiperObjectToPubValue(
      piper::Runtime& rt, const piper::Object& obj,
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
  static bool IsBigInt(piper::Runtime& rt, const piper::Object& obj);

  static bool ConvertBigIntToStringIfNecessary(piper::Runtime& rt,
                                               const piper::Object& obj,
                                               std::string& result);
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_VALUE_WRAPPER_VALUE_WRAPPER_UTILS_H_
