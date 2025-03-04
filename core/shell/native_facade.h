// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_NATIVE_FACADE_H_
#define CORE_SHELL_NATIVE_FACADE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/public/prop_bundle.h"
#include "core/renderer/data/template_data.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/jsi/api_call_back.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_timing.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/services/timing_handler/timing.h"
#include "core/shell/common/platform_call_back_manager.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {

namespace base {
struct LynxError;
}

namespace shell {

class NativeFacade {
 public:
  NativeFacade() = default;
  virtual ~NativeFacade() = default;

  NativeFacade(const NativeFacade& facade) = delete;
  NativeFacade& operator=(const NativeFacade&) = delete;

  NativeFacade(NativeFacade&& facade) = default;
  NativeFacade& operator=(NativeFacade&&) = default;

  virtual void OnDataUpdated() = 0;

  virtual void OnTasmFinishByNative() = 0;

  virtual void OnTemplateLoaded(const std::string& url) = 0;

  virtual void OnRuntimeReady() = 0;

  virtual void ReportError(const base::LynxError& error) = 0;

  virtual void OnModuleMethodInvoked(const std::string& module,
                                     const std::string& method,
                                     int32_t code) = 0;

  // TODO(huzhanbo.luc): remove this later
  virtual void OnFirstLoadPerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing) {}

  virtual void OnUpdatePerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing) {}

  virtual void OnTimingSetup(const lepus::Value& timing_info) = 0;

  virtual void OnTimingUpdate(const lepus::Value& timing_info,
                              const lepus::Value& update_timing,
                              const std::string& update_flag) = 0;

  virtual void OnDynamicComponentPerfReady(const lepus::Value& perf_info) = 0;

  virtual void OnConfigUpdated(const lepus::Value& data) = 0;

  virtual void TriggerLepusMethodAsync(const std::string& method_name,
                                       const lepus::Value& argus) = 0;

  virtual void InvokeUIMethod(const tasm::LynxGetUIResult& ui_result,
                              const std::string& method,
                              std::unique_ptr<tasm::PropBundle> params,
                              piper::ApiCallBack callback) = 0;

  virtual void FlushJSBTiming(piper::NativeModuleInfo timing) = 0;

  virtual void OnSSRHydrateFinished(const std::string& url){};

  virtual void OnPageChanged(bool is_first_screen) {}

  virtual void OnUpdateDataWithoutChange(){};

  virtual void OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) {}

  // TODO(songshourui.null): override this function later.
  virtual void OnReceiveMessageEvent(runtime::MessageEvent event) {}

  void InvokePlatformCallBackWithValue(
      const std::shared_ptr<shell::PlatformCallBackHolder>& callback,
      const lepus::Value& value) {
    call_back_manager_.InvokeWithValue(callback, value);
  };

  void RemovePlatformCallBack(
      const std::shared_ptr<shell::PlatformCallBackHolder>& callback) {
    call_back_manager_.EraseCallBack(callback);
  };

  std::shared_ptr<PlatformCallBackHolder> CreatePlatformCallBackHolder(
      std::unique_ptr<PlatformCallBack> call_back) {
    return call_back_manager_.CreatePlatformCallBackHolder(
        std::move(call_back));
  }

 protected:
  PlatformCallBackManager call_back_manager_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_NATIVE_FACADE_H_
