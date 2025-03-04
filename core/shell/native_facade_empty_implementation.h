// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_NATIVE_FACADE_EMPTY_IMPLEMENTATION_H_
#define CORE_SHELL_NATIVE_FACADE_EMPTY_IMPLEMENTATION_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/data/template_data.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/page_config.h"
#include "core/runtime/bindings/jsi/api_call_back.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_timing.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/services/timing_handler/timing.h"
#include "core/shell/native_facade.h"
#include "core/shell/native_facade_reporter.h"

namespace lynx {
namespace base {
struct LynxError;
}
namespace shell {

class NativeFacadeEmptyImpl : public NativeFacade, NativeFacadeReporter {
 public:
  NativeFacadeEmptyImpl() = default;
  virtual ~NativeFacadeEmptyImpl() = default;

  NativeFacadeEmptyImpl(const NativeFacadeEmptyImpl& facade) = delete;
  NativeFacadeEmptyImpl& operator=(const NativeFacadeEmptyImpl&) = delete;

  NativeFacadeEmptyImpl(NativeFacadeEmptyImpl&& facade) = default;
  NativeFacadeEmptyImpl& operator=(NativeFacadeEmptyImpl&&) = default;

  virtual void OnDataUpdated() override {}

  virtual void OnTasmFinishByNative() override {}

  virtual void OnTemplateLoaded(const std::string& url) override {}

  virtual void OnRuntimeReady() override {}

  virtual void ReportError(const base::LynxError& error) override {}

  virtual void OnModuleMethodInvoked(const std::string& module,
                                     const std::string& method,
                                     int32_t code) override {}

  virtual void OnTimingSetup(const lepus::Value& timing_info) override {}

  virtual void OnTimingUpdate(const lepus::Value& timing_info,
                              const lepus::Value& update_timing,
                              const std::string& update_flag) override {}

  virtual void OnPerformanceEvent(const lepus::Value& entry) override{};

  virtual void OnDynamicComponentPerfReady(
      const lepus::Value& perf_info) override {}

  virtual void OnConfigUpdated(const lepus::Value& data) override {}

  virtual void TriggerLepusMethodAsync(const std::string& method_name,
                                       const lepus::Value& argus) override {}

  virtual void InvokeUIMethod(const tasm::LynxGetUIResult& ui_result,
                              const std::string& method,
                              std::unique_ptr<tasm::PropBundle> params,
                              piper::ApiCallBack callback) override {}

  virtual void FlushJSBTiming(piper::NativeModuleInfo timing) override {}
};

}  // namespace shell
}  // namespace lynx
#endif  // CORE_SHELL_NATIVE_FACADE_EMPTY_IMPLEMENTATION_H_
