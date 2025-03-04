// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/ios/tasm_platform_invoker_darwin.h"

#include "core/runtime/bindings/lepus/ios/lynx_lepus_module_darwin.h"

#include "base/include/string/string_utils.h"

namespace lynx {
namespace shell {

void TasmPlatformInvokerDarwin::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render setPageConfig:config];
}

std::string TasmPlatformInvokerDarwin::TranslateResourceForTheme(const std::string& res_id,
                                                                 const std::string& theme_key) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  if (res_id.empty() || ![render respondsToSelector:@selector(translatedResourceWithId:
                                                                              themeKey:)]) {
    return std::string();
  }

  NSString* resId = [NSString stringWithUTF8String:res_id.c_str()];
  NSString* key = theme_key.empty() ? nil : [NSString stringWithUTF8String:theme_key.c_str()];
  return base::SafeStringConvert([[render translatedResourceWithId:resId themeKey:key] UTF8String]);
}

lepus::Value TasmPlatformInvokerDarwin::TriggerLepusMethod(const std::string& js_method_name,
                                                           const lepus::Value& args) {
  lepus::Value value = lynx::piper::TriggerLepusMethod(js_method_name, args, _render);
  if (!value.IsNil()) {
    return value;
  }
  return lepus::Value();
}

void TasmPlatformInvokerDarwin::TriggerLepusMethodAsync(const std::string& method_name,
                                                        const lepus::Value& args) {
  lynx::piper::TriggerLepusMethodAsync(method_name, args, _render);
}

void TasmPlatformInvokerDarwin::GetI18nResource(const std::string& channel,
                                                const std::string& fallback_url) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  NSString* ns_channel = [NSString stringWithUTF8String:channel.c_str()];
  NSString* ns_fallback_url = [NSString stringWithUTF8String:fallback_url.c_str()];
  [render getI18nResourceForChannel:ns_channel withFallbackUrl:ns_fallback_url];
}

}  // namespace shell
}  // namespace lynx
