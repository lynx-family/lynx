// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/harmony/tasm_platform_invoker_harmony.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"

namespace lynx {
namespace harmony {

void TasmPlatformInvokerHarmony::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  fml::TaskRunner::RunNowOrPostTask(
      ui_task_runner_, [weak_flag = weak_flag_, config]() {
        auto flag = weak_flag.lock();
        if (!flag) {
          return;
        }
        flag->renderer->OnPageConfigDecoded(config);
      });
}

std::string TasmPlatformInvokerHarmony::TranslateResourceForTheme(
    const std::string& res_id, const std::string& theme_key) {
  return std::string();
}

lepus::Value TasmPlatformInvokerHarmony::TriggerLepusMethod(
    const std::string& js_method_name, const lepus::Value& args) {
  return lepus::Value();
}

void TasmPlatformInvokerHarmony::TriggerLepusMethodAsync(
    const std::string& method_name, const lepus::Value& args) {}

void TasmPlatformInvokerHarmony::GetI18nResource(
    const std::string& channel, const std::string& fallback_url) {}

}  // namespace harmony
}  // namespace lynx
