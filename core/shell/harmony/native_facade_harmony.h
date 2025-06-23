// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HARMONY_NATIVE_FACADE_HARMONY_H_
#define CORE_SHELL_HARMONY_NATIVE_FACADE_HARMONY_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/shell/lynx_shell.h"
#include "core/shell/native_facade.h"

namespace lynx {
namespace harmony {
class LynxTemplateRenderer;

class NativeFacadeHarmony : public shell::NativeFacade {
 public:
  explicit NativeFacadeHarmony(LynxTemplateRenderer* renderer);
  ~NativeFacadeHarmony() override;

  NativeFacadeHarmony(const NativeFacadeHarmony& facade) = delete;
  NativeFacadeHarmony& operator=(const NativeFacadeHarmony&) = delete;

  NativeFacadeHarmony(NativeFacadeHarmony&& facade) = default;
  NativeFacadeHarmony& operator=(NativeFacadeHarmony&&) = default;

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
                      piper::ApiCallBack callback) override;

  void FlushJSBTiming(piper::NativeModuleInfo timing) override;

  void OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) override;

  void OnEventCapture(long target_id, bool is_catch, int64_t event_id) override;

  void OnEventBubble(long target_id, bool is_catch, int64_t event_id) override;

  void OnEventFire(long target_id, bool is_stop, int64_t event_id) override;

 private:
  LynxTemplateRenderer* renderer_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // CORE_SHELL_HARMONY_NATIVE_FACADE_HARMONY_H_
