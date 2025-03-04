// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/java_script_element.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/common/utils.h"

namespace lynx {
namespace piper {
Value JavaScriptElement::get(lynx::piper::Runtime *rt,
                             const lynx::piper::PropNameID &name) {
  auto methodName = name.utf8(*rt);

  if (methodName == "animate") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "animate"), 4,
        [this](Runtime &rt, const piper::Value &this_val,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          if (count < 4) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "NativeElement.animate args count must be 4"));
          }
          auto ptr = native_app_.lock();
          if (ptr) {
            auto props = lepus::CArray::Create();
            auto maybe_operation = args[0].asNumber(rt);
            if (!maybe_operation) {
              return base::unexpected(
                  BUILD_JSI_NATIVE_EXCEPTION("Args[0] must be a number."));
            }
            int32_t operation = static_cast<int32_t>(*maybe_operation);
            props->emplace_back(operation);

            if (args[1].isString()) {
              props->emplace_back(args[1].getString(rt).utf8(rt));
            }

            if (operation == AnimationOperation::START && args[2].isObject()) {
              auto value = ptr->ParseJSValueToLepusValue(
                  args[2], root_id_ == "card" ? "-1" : root_id_);
              if (!value) {
                return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                    "ParseJSValueToLepusValue error in animate args[2]"));
              }
              props->emplace_back(std::move(*value));
            }

            if (operation == AnimationOperation::START && args[3].isObject()) {
              auto value = ptr->ParseJSValueToLepusValue(
                  args[3], root_id_ == "card" ? "-1" : root_id_);
              if (!value) {
                return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                    "ParseJSValueToLepusValue error in animate args[3]"));
              }
              props->emplace_back(std::move(*value));
            }

            ptr->ElementAnimate(root_id_, selector_id_,
                                lepus::Value(std::move(props)));
          }
          return piper::Value::undefined();
        });
  }

  if (methodName == "setProperty") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "setProperty"), 2,
        [this](Runtime &rt, const piper::Value &this_val,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          auto ptr = native_app_.lock();
          if (count < 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "lynx.setProperty args is empty! args count is 0."));
          }
          auto lepus_value_opt =
              ptr->ParseJSValueToLepusValue(std::move(args[0]), PAGE_GROUP_ID);
          if (!lepus_value_opt) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("ParseJSValueToLepusValue error in "
                                           "java_script_element setProperty."));
          }
          ptr->SetCSSVariable(root_id_, selector_id_,
                              std::move(*lepus_value_opt));
          return piper::Value::undefined();
        });
  }
  return piper::Value::undefined();
}

void JavaScriptElement::set(Runtime *, const PropNameID &name,
                            const Value &value) {}

std::vector<PropNameID> JavaScriptElement::getPropertyNames(Runtime &rt) {
  std::vector<PropNameID> vec;
  vec.push_back(piper::PropNameID::forUtf8(rt, "animate"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "setProperty"));
  return vec;
}
}  // namespace piper
}  // namespace lynx
