// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/module/lynx_ui_method_module.h"

#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/log/logging.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace harmony {

const std::string LynxUIMethodModule::name_ = "LynxUIMethodModule";

LynxUIMethodModule::LynxUIMethodModule(
    const std::shared_ptr<tasm::harmony::LynxContext>& context)
    : NativeModuleCAPI(), context_(context) {
  RegisterMethod(
      piper::NativeModuleMethod("invokeUIMethod", 5),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxUIMethodModule::InvokeUIMethod));
}

void LynxUIMethodModule::Destroy() { LOGI("LynxUIMethodModule Destroy"); }

std::unique_ptr<pub::Value> LynxUIMethodModule::InvokeUIMethod(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  auto delegate = delegate_.lock();
  if (!lepus_args.IsArray() || !ctx || !delegate) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  auto args_array = lepus_args.Array();
  const auto& component = args_array->get(0).StdString();
  auto node = args_array->get(1).Array()->get(0).StdString();
  if (node[0] == '#') {
    node = node.substr(1);
  }
  const auto& method = args_array->get(2).StdString();
  auto it = callbacks.find(4);
  if (it == callbacks.end()) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback =
      [delegate, callback = it->second](int32_t code,
                                        const lepus::Value& data) {
        auto callback_args = lepus::CArray::Create();
        auto ret = lepus::Dictionary::Create();
        ret->SetValue("code", code);
        ret->SetValue("data", data);
        callback_args->emplace_back(ret);

        callback->SetArgs(
            std::make_unique<PubLepusValue>(lepus::Value(callback_args)));
        delegate->InvokeCallback(callback);
      };
  ctx->InvokeUIMethod(component, node, method, args_array->get(3),
                      std::move(callback));

  return std::unique_ptr<pub::Value>(nullptr);
}

}  // namespace harmony
}  // namespace lynx
