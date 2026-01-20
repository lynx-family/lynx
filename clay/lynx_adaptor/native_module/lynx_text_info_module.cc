// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_text_info_module.h"

#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/ui/shadow/text_render.h"

namespace lynx {
const std::string LynxTextInfoModule::name_ = "LynxTextInfoModule";

LynxTextInfoModule::LynxTextInfoModule(uint32_t view_context_id) {
  view_context_id_ = view_context_id;
  lynx::runtime::NativeModuleMethod get_text_info("getTextInfo", 2);
  // RegisterMethod(get_text_info, &LynxTextInfoModule::GetTextInfo);
  auto method = runtime::NativeModuleMethod("getTextInfo", 2);
  methods_.emplace("getTextInfo", method);
}

std::unique_ptr<pub::Value> LynxTextInfoModule::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  auto rk_info = ValueConverter::CreateClayValue(info);
  auto result = clay::TextRender::GetTextInfo(content.c_str(), rk_info);
  return std::make_unique<ClayValue>(std::move(result));
}

base::expected<std::unique_ptr<pub::Value>, std::string>
LynxTextInfoModule::InvokeMethod(const std::string& method_name,
                                 std::unique_ptr<pub::Value> args, size_t count,
                                 const runtime::CallbackMap& callbacks) {
  if (method_name == "getTextInfo") {
    if (!args->IsArray() || count != 2) {
      return base::unexpected("Invalid argument count");
    }
    auto content = args->GetValueAtIndex(0)->str();
    auto info = args->GetValueAtIndex(1);
    return GetTextInfo(content, *info);
  } else {
    return base::unexpected("Unknown method");
  }
}

}  // namespace lynx
