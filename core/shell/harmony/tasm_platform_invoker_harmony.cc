// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/harmony/tasm_platform_invoker_harmony.h"

#include <atomic>

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"

namespace lynx {
namespace harmony {

void TasmPlatformInvokerHarmony::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  fml::TaskRunner::RunNowOrPostTask(
      ui_task_runner_, [weak_flag = weak_flag_, config]() {
        auto flag = weak_flag.lock();
        auto* renderer =
            flag ? flag->renderer.load(std::memory_order_acquire) : nullptr;
        if (!renderer) {
          return;
        }
        renderer->OnPageConfigDecoded(config);
      });
}

std::string TasmPlatformInvokerHarmony::TranslateResourceForTheme(
    const std::string& res_id, const std::string& theme_key) {
  return std::string();
}

lepus::Value TasmPlatformInvokerHarmony::TriggerLepusMethod(
    const std::string& method_name, const lepus::Value& args) {
  lepus::Value ret;
  ui_task_runner_->PostSyncTask(
      [weak_flag = weak_flag_, method_name, args, &ret]() {
        auto flag = weak_flag.lock();
        auto* renderer =
            flag ? flag->renderer.load(std::memory_order_acquire) : nullptr;
        if (!renderer) {
          return;
        }
        ret = renderer->TriggerLepusMethod(method_name, args);
      });
  return ret;
}

void TasmPlatformInvokerHarmony::TriggerLepusMethodAsync(
    const std::string& method_name, const lepus::Value& args) {
  fml::TaskRunner::RunNowOrPostTask(
      ui_task_runner_, [weak_flag = weak_flag_, method_name, args]() {
        auto flag = weak_flag.lock();
        auto* renderer =
            flag ? flag->renderer.load(std::memory_order_acquire) : nullptr;
        if (!renderer) {
          return;
        }
        renderer->TriggerLepusMethodAsync(method_name, args);
      });
}

void TasmPlatformInvokerHarmony::GetI18nResource(
    const std::string& channel, const std::string& fallback_url) {}

}  // namespace harmony
}  // namespace lynx
