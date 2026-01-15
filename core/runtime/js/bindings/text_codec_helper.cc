// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/bindings/text_codec_helper.h"

#include <string>
#include <vector>

namespace lynx {
namespace piper {
Value TextCodecHelper::get(Runtime* rt, const PropNameID& name) {
  auto methodName = name.utf8(*rt);

  if (methodName == "decode") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "decode"), 0,
        [](Runtime& rt, const Value& thisVal, const Value* args,
           size_t count) -> base::expected<Value, JSINativeException> {
          if (count == 0) {
            return piper::String::createFromUtf8(rt, "");
          }

          if (count != 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "TextDecoder().decode only support arraybuffer"));
          }

          const auto& obj = args[0];
          if (!obj.isObject() || !obj.asObject(rt)->isArrayBuffer(rt)) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "TextDecoder().decode only support arraybuffer"));
          }

          const auto& buffer = obj.asObject(rt)->getArrayBuffer(rt);
          return piper::String::createFromUtf8(rt, buffer.data(rt),
                                               buffer.size(rt));
        });
  }

  if (methodName == "encode") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "encode"), 0,
        [](Runtime& rt, const Value& thisVal, const Value* args,
           size_t count) -> base::expected<Value, JSINativeException> {
          if (count != 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "TextEncoder().decode only support string"));
          }

          const auto& obj = args[0];
          if (!obj.isString()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "TextEncoder().decode only support string"));
          }
          const auto& string = obj.toString(rt)->utf8(rt);
          return piper::ArrayBuffer(
              rt, reinterpret_cast<const uint8_t*>(string.c_str()),
              string.size());
        });
  }

  return piper::Value::undefined();
}

std::vector<PropNameID> TextCodecHelper::getPropertyNames(Runtime& rt) {
  std::vector<PropNameID> vec;
  vec.push_back(piper::PropNameID::forUtf8(rt, "decode"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "encode"));
  return vec;
}

}  // namespace piper
}  // namespace lynx
