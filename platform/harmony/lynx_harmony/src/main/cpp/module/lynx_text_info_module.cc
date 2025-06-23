// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/module/lynx_text_info_module.h"

#include <string>
#include <unordered_map>
#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/text_utils.h"

namespace lynx {
namespace harmony {

const std::string LynxTextInfoModule::name_ = "LynxTextInfoModule";

LynxTextInfoModule::LynxTextInfoModule(
    const std::shared_ptr<tasm::harmony::LynxContext>& context)
    : NativeModuleCAPI(), context_(context) {
  RegisterMethod(
      piper::NativeModuleMethod("getTextInfo", 2),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxTextInfoModule::GetTextInfo));
}

base::expected<std::unique_ptr<pub::Value>, std::string>
LynxTextInfoModule::InvokeMethod(const std::string& method_name,
                                 std::unique_ptr<pub::Value> args, size_t count,
                                 const piper::CallbackMap& callbacks) {
  if (method_name == "getTextInfo") {
    return GetTextInfo(std::move(args), callbacks);
  }
  return base::unexpected(
      std::string("LynxTextInfoModule::InvokeMethod method_name:") +
      method_name + " not found");
}

std::unique_ptr<pub::Value> LynxTextInfoModule::GetTextInfo(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto ctx = context_.lock();
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  if (!args->IsArray() && args->Length() != 2 && ctx == nullptr) {
    return std::unique_ptr<pub::Value>(nullptr);
  }

  const auto& content = args->GetValueAtIndex(0)->str();
  auto info = args->GetValueAtIndex(1);
  const pub::Value& info_ref = *info;
  auto lepus_result =
      tasm::harmony::TextUtils::GetTextInfo(content, info_ref, ctx.get());
  return std::make_unique<PubLepusValue>(std::move(lepus_result));
}

void LynxTextInfoModule::Destroy() { LOGI("LynxTextInfoModule Destroy"); }

}  // namespace harmony
}  // namespace lynx
