// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/harmony/tasm_platform_invoker_harmony.h"

#include <atomic>
#include <optional>

#include "base/include/log/logging.h"
#include "core/base/harmony/vsync_monitor_harmony.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/lynx_env.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace harmony {
namespace {

class PageVSyncMonitor final : public base::VSyncMonitorHarmony {
 public:
  explicit PageVSyncMonitor(
      const std::weak_ptr<tasm::harmony::LynxContext>& context)
      : context_(context) {}

 private:
  void RequestVSync() override {
    base::UIThread::GetRunner()->PostTask(
        [weak_context = context_, weak_monitor = weak_from_this()]() {
          auto context = weak_context.lock();
          auto* owner = context ? context->GetUIOwner() : nullptr;
          if (!owner || owner->Destroyed() || weak_monitor.expired()) {
            return;
          }
          owner->RequestAnimationFrame(
              [weak_monitor](std::optional<int64_t> timestamp) {
                if (auto monitor = weak_monitor.lock()) {
                  auto& page_monitor = static_cast<PageVSyncMonitor&>(*monitor);
                  auto context = page_monitor.context_.lock();
                  auto* owner = context ? context->GetUIOwner() : nullptr;
                  if (!owner || owner->Destroyed()) {
                    return;
                  }
                  if (timestamp) {
                    page_monitor.OnUIFrame(*timestamp);
                  } else {
                    LOGE("ArkUI VSync request failed");
                    page_monitor.DiscardPendingVSync();
                  }
                }
              });
        });
  }

  const std::weak_ptr<tasm::harmony::LynxContext> context_;
};

}  // namespace

std::shared_ptr<base::VSyncMonitor>
TasmPlatformInvokerHarmony::CreatePageVSyncMonitor() const {
  if (weak_context_.expired() ||
      !tasm::LynxEnv::GetInstance().EnableAnimationVsyncOnUIThread()) {
    return nullptr;
  }
  return std::make_shared<PageVSyncMonitor>(weak_context_);
}

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
  ui_task_runner_->PostSyncTask([weak_flag = weak_flag_, method_name,
                                 safe_args = lepus::Value::ShallowCopy(args),
                                 &ret]() {
    auto flag = weak_flag.lock();
    auto* renderer =
        flag ? flag->renderer.load(std::memory_order_acquire) : nullptr;
    if (!renderer) {
      return;
    }
    ret = renderer->TriggerLepusMethod(method_name, safe_args);
  });
  return ret;
}

void TasmPlatformInvokerHarmony::TriggerLepusMethodAsync(
    const std::string& method_name, const lepus::Value& args) {
  fml::TaskRunner::RunNowOrPostTask(
      ui_task_runner_, [weak_flag = weak_flag_, method_name,
                        safe_args = lepus::Value::ShallowCopy(args)]() {
        auto flag = weak_flag.lock();
        auto* renderer =
            flag ? flag->renderer.load(std::memory_order_acquire) : nullptr;
        if (!renderer) {
          return;
        }
        renderer->TriggerLepusMethodAsync(method_name, safe_args);
      });
}

void TasmPlatformInvokerHarmony::GetI18nResource(
    const std::string& channel, const std::string& fallback_url) {}

}  // namespace harmony
}  // namespace lynx
