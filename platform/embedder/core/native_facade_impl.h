// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_CORE_NATIVE_FACADE_IMPL_H_
#define PLATFORM_EMBEDDER_CORE_NATIVE_FACADE_IMPL_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/shell/lynx_shell.h"
#include "core/shell/native_facade.h"

namespace lynx {
namespace embedder {

class LynxTemplateRenderer;

class NativeFacadeImpl : public shell::NativeFacade {
 public:
  explicit NativeFacadeImpl(LynxTemplateRenderer* renderer);
  ~NativeFacadeImpl() override;

  NativeFacadeImpl(const NativeFacadeImpl& facade) = delete;
  NativeFacadeImpl& operator=(const NativeFacadeImpl&) = delete;

  NativeFacadeImpl(NativeFacadeImpl&& facade) = default;
  NativeFacadeImpl& operator=(NativeFacadeImpl&&) = default;

  void OnDataUpdated() override;

  void OnPageChanged(bool is_first_screen) override;

  void OnTasmFinishByNative() override;

  void OnTemplateLoaded(const std::string& url) override;

  void OnRuntimeReady() override;

  void ReportError(const base::LynxError& error) override;

  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t code) override;

  void OnFirstLoadPerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing) override;

  void OnUpdatePerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing) override;

  void OnTimingSetup(const lepus::Value& timing_info) override;

  void OnTimingUpdate(const lepus::Value& timing_info,
                      const lepus::Value& update_timing,
                      const std::string& update_flag) override;

  void OnDynamicComponentPerfReady(const lepus::Value& perf_info) override;

  void OnConfigUpdated(const lepus::Value& data) override;

  void OnUpdateDataWithoutChange() override;

  void TriggerLepusMethodAsync(const std::string& js_method_name,
                               const lepus::Value& args) override;

  void InvokeUIMethod(const tasm::LynxGetUIResult& ui_result,
                      const std::string& method,
                      fml::RefPtr<tasm::PropBundle> params,
                      runtime::js::ApiCallBack callback) override;

  void FlushJSBTiming(runtime::js::NativeModuleInfo timing) override;

  void OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) override;

  void OnEventCapture(long target_id, bool is_catch, int64_t event_id) override;

  void OnEventBubble(long target_id, bool is_catch, int64_t event_id) override;

  void OnEventFire(long target_id, bool is_stop, int64_t event_id) override;

  void OnLynxEvent(const lepus::Value& event_detail) override;

 private:
  LynxTemplateRenderer* renderer_ = nullptr;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CORE_NATIVE_FACADE_IMPL_H_
