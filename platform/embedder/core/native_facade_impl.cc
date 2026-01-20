// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/core/native_facade_impl.h"

#include "platform/embedder/core/lynx_template_renderer.h"

namespace lynx {
namespace embedder {

NativeFacadeImpl::NativeFacadeImpl(LynxTemplateRenderer* renderer)
    : renderer_(renderer) {}

NativeFacadeImpl::~NativeFacadeImpl() {}

void NativeFacadeImpl::OnDataUpdated() { renderer_->OnDataUpdated(); }

void NativeFacadeImpl::OnPageChanged(bool is_first_screen) {
  renderer_->OnPageChanged(is_first_screen);
}

void NativeFacadeImpl::OnTasmFinishByNative() {}

void NativeFacadeImpl::OnTemplateLoaded(const std::string& url) {
  renderer_->OnLoaded(url);
}

void NativeFacadeImpl::OnRuntimeReady() { renderer_->OnRuntimeReady(); }

void NativeFacadeImpl::ReportError(const base::LynxError& error) {
  renderer_->OnErrorOccurred(static_cast<int>(error.error_level_),
                             error.error_code_, error.error_message_,
                             error.fix_suggestion_, error.custom_info_,
                             error.is_logbox_only_);
}

void NativeFacadeImpl::OnModuleMethodInvoked(const std::string& module,
                                             const std::string& method,
                                             int32_t code) {}

void NativeFacadeImpl::OnFirstLoadPerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  renderer_->OnFirstLoadPerfReady(perf, perf_timing);
}

void NativeFacadeImpl::OnUpdatePerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  renderer_->OnUpdatePerfReady(perf, perf_timing);
}

void NativeFacadeImpl::OnTimingSetup(const lepus::Value& timing_info) {
  renderer_->OnTimingSetup(timing_info);
}
void NativeFacadeImpl::OnTimingUpdate(const lepus::Value& timing_info,
                                      const lepus::Value& update_timing,
                                      const std::string& update_flag) {
  renderer_->OnTimingUpdate(timing_info, update_timing, update_flag);
}

void NativeFacadeImpl::OnDynamicComponentPerfReady(
    const lepus::Value& perf_info) {}

void NativeFacadeImpl::OnConfigUpdated(const lepus::Value& data) {
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

void NativeFacadeImpl::TriggerLepusMethodAsync(
    const std::string& js_method_name, const lepus::Value& args) {}

void NativeFacadeImpl::OnUpdateDataWithoutChange() {}

void NativeFacadeImpl::InvokeUIMethod(const tasm::LynxGetUIResult& ui_result,
                                      const std::string& method,
                                      fml::RefPtr<tasm::PropBundle> params,
                                      runtime::js::ApiCallBack callback) {}

void NativeFacadeImpl::FlushJSBTiming(runtime::js::NativeModuleInfo timing) {}

void NativeFacadeImpl::OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) {
  renderer_->OnTemplateBundleReady(bundle);
}

void NativeFacadeImpl::OnEventCapture(long target_id, bool is_catch,
                                      int64_t event_id) {}

void NativeFacadeImpl::OnEventBubble(long target_id, bool is_catch,
                                     int64_t event_id) {}

void NativeFacadeImpl::OnEventFire(long target_id, bool is_stop,
                                   int64_t event_id) {}

void NativeFacadeImpl::OnLynxEvent(const lepus::Value& event_detail) {}

}  // namespace embedder
}  // namespace lynx
