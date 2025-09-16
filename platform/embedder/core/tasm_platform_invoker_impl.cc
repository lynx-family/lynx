// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/core/tasm_platform_invoker_impl.h"

#include "platform/embedder/core/lynx_template_renderer.h"

namespace lynx {
namespace embedder {

void TasmPlatformInvokerImpl::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  fml::TaskRunner::RunNowOrPostTask(
      base::UIThread::GetRunner(), [weak_renderer = weak_renderer_, config]() {
        if (auto renderer = weak_renderer.lock()) {
          renderer->renderer->OnPageConfigDecoded(config);
        }
      });
}

std::string TasmPlatformInvokerImpl::TranslateResourceForTheme(
    const std::string& res_id, const std::string& theme_key) {
  return std::string();
}

lepus::Value TasmPlatformInvokerImpl::TriggerLepusMethod(
    const std::string& js_method_name, const lepus::Value& args) {
  return lepus::Value();
}

void TasmPlatformInvokerImpl::TriggerLepusMethodAsync(
    const std::string& method_name, const lepus::Value& args) {}

void TasmPlatformInvokerImpl::GetI18nResource(const std::string& channel,
                                              const std::string& fallback_url) {
}

}  // namespace embedder
}  // namespace lynx
