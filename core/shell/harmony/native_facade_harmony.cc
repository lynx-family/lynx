// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/harmony/native_facade_harmony.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"

namespace lynx {
namespace harmony {

NativeFacadeHarmony::NativeFacadeHarmony(LynxTemplateRenderer* renderer)
    : renderer_(renderer) {}

NativeFacadeHarmony::~NativeFacadeHarmony() {}

void NativeFacadeHarmony::OnDataUpdated() { renderer_->OnDataUpdated(); }

void NativeFacadeHarmony::OnPageChanged(bool is_first_screen) {
  renderer_->OnPageChanged(is_first_screen);
}

void NativeFacadeHarmony::OnTasmFinishByNative() {}

void NativeFacadeHarmony::OnTemplateLoaded(const std::string& url) {
  renderer_->OnLoaded(url);
}

void NativeFacadeHarmony::OnRuntimeReady() { renderer_->OnRuntimeReady(); }

void NativeFacadeHarmony::ReportError(const base::LynxError& error) {
  renderer_->OnErrorOccurred(static_cast<int>(error.error_level_),
                             error.error_code_, error.error_message_,
                             error.fix_suggestion_, error.custom_info_,
                             error.is_logbox_only_);
}

void NativeFacadeHarmony::OnModuleMethodInvoked(const std::string& module,
                                                const std::string& method,
                                                int32_t code) {}

void NativeFacadeHarmony::OnFirstLoadPerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  renderer_->OnFirstLoadPerfReady(perf, perf_timing);
}

void NativeFacadeHarmony::OnUpdatePerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  renderer_->OnUpdatePerfReady(perf, perf_timing);
}

void NativeFacadeHarmony::OnTimingSetup(const lepus::Value& timing_info) {
  //  renderer_->OnTimingSetup(timing_info);
}
void NativeFacadeHarmony::OnTimingUpdate(const lepus::Value& timing_info,
                                         const lepus::Value& update_timing,
                                         const std::string& update_flag) {
  //  renderer_->OnTimingUpdate(timing_info, update_timing, update_flag);
}

void NativeFacadeHarmony::OnDynamicComponentPerfReady(
    const lepus::Value& perf_info) {}

void NativeFacadeHarmony::OnConfigUpdated(const lepus::Value& data) {
  if (data.IsTable() && data.Table()->size() > 0) {
    for (const auto& prop : *(data.Table())) {
      if (!prop.first.IsEqual(tasm::CARD_CONFIG_THEME) ||
          !prop.second.IsTable()) {
        continue;
      }
      auto& value = prop.second;
      std::unordered_map<std::string, std::string> configs;
      for (const auto& theme_prop : *(value.Table())) {
        if (theme_prop.second.IsString()) {
          configs.emplace(theme_prop.first.str(),
                          theme_prop.second.StdString());
          renderer_->OnThemeUpdatedByJs(configs);
        }
      }
    }
  }
}

void NativeFacadeHarmony::TriggerLepusMethodAsync(
    const std::string& js_method_name, const lepus::Value& args) {}

void NativeFacadeHarmony::OnUpdateDataWithoutChange() {}

void NativeFacadeHarmony::InvokeUIMethod(const tasm::LynxGetUIResult& ui_result,
                                         const std::string& method,
                                         fml::RefPtr<tasm::PropBundle> params,
                                         piper::ApiCallBack callback) {}

void NativeFacadeHarmony::FlushJSBTiming(piper::NativeModuleInfo timing) {}

void NativeFacadeHarmony::OnTemplateBundleReady(
    tasm::LynxTemplateBundle bundle) {
  renderer_->OnTemplateBundleReady(bundle);
}

void NativeFacadeHarmony::OnEventCapture(long target_id, bool is_catch,
                                         int64_t event_id) {}

void NativeFacadeHarmony::OnEventBubble(long target_id, bool is_catch,
                                        int64_t event_id) {}

void NativeFacadeHarmony::OnEventFire(long target_id, bool is_stop,
                                      int64_t event_id) {}

}  // namespace harmony
}  // namespace lynx
