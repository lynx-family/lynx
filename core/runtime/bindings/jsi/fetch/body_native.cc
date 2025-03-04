// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/fetch/body_native.h"

#include <memory>
#include <utility>
#include <vector>

#include "core/runtime/bindings/jsi/js_app.h"

namespace lynx {
namespace piper {

Value BodyNative::SafeUseBody(Runtime& rt,
                              base::MoveOnlyClosure<Value, Runtime&> use) {
  if (body_used_) {
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION("body is used"));
    return Value::undefined();
  }
  auto result = use(rt);
  body_used_ = true;
  data_.clear();
  return result;
}

Value BodyNative::get(Runtime* rt, const PropNameID& name) {
  auto method_name = name.utf8(*rt);
  if (method_name == "bodyUsed") {
    return Value(body_used_);
  } else if (method_name == "clone") {
    if (body_used_) {
      rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION("body is used"));
      return Value::undefined();
    }
    auto body_native = std::make_shared<BodyNative>(data_);
    return Object::createFromHostObject(*rt, std::move(body_native));
  } else if (method_name == "text") {
    return SafeUseBody(*rt, [this](Runtime& rt) mutable {
      auto text = String::createFromAscii(
          rt, reinterpret_cast<const char*>(data_.data()), data_.size());
      return text;
    });
  } else if (method_name == "json") {
    return SafeUseBody(*rt, [this](Runtime& rt) mutable {
      auto json_object =
          Value::createFromJsonUtf8(rt, data_.data(), data_.size());
      if (!json_object) {
        rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION("JSON.parse failed"));
        return Value::undefined();
      }
      return std::move(*json_object);
    });
  } else if (method_name == "arrayBuffer") {
    return SafeUseBody(*rt, [this](Runtime& rt) mutable {
      auto array_buffer = ArrayBuffer(rt, data_.data(), data_.size());
      return Value(array_buffer);
    });
  }

  return Value::undefined();
}

void BodyNative::RegisterBodyNative(Runtime& rt) {
  auto global = rt.global();
  if (!global.hasProperty(rt, "CreateBodyNative")) {
    global.setProperty(
        rt, "CreateBodyNative",
        Function::createFromHostFunction(
            rt, PropNameID::forAscii(rt, "CreateBodyNative"), 1,
            [](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
              const auto& body_init = args[0].asObject(rt);
              const auto& body_init_obj = *body_init;
              const auto& body_data = body_init_obj.getProperty(rt, "bodyData");

              auto is_array_buffer =
                  body_init_obj.getProperty(rt, "isArrayBuffer");

              if (is_array_buffer && is_array_buffer->getBool()) {
                const auto& body_data_buffer =
                    body_data->asObject(rt)->getArrayBuffer(rt);
                auto size = body_data_buffer.size(rt);
                auto data = body_data_buffer.data(rt);

                auto body_native = std::make_shared<BodyNative>(
                    std::vector<uint8_t>(data, data + size));
                return Object::createFromHostObject(rt, std::move(body_native));
              } else if (body_data && !body_data->isUndefined()) {
                const auto& body_data_str = body_data->toString(rt)->utf8(rt);
                auto body_native = std::make_shared<BodyNative>(body_data_str);
                return Object::createFromHostObject(rt, std::move(body_native));
              } else {
                auto body_native = std::make_shared<BodyNative>("");
                return Object::createFromHostObject(rt, std::move(body_native));
              }
            }));
  }
}

}  // namespace piper
}  // namespace lynx
