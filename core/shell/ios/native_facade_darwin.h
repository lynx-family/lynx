// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_NATIVE_FACADE_DARWIN_H_
#define CORE_SHELL_IOS_NATIVE_FACADE_DARWIN_H_

#import <Foundation/Foundation.h>
#import <Lynx/LynxView.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/shell/native_facade.h"
#import "platform/darwin/common/lynx/TemplateRenderCallbackProtocol.h"

namespace lynx {
namespace shell {

class NativeFacadeDarwin : public NativeFacade {
 public:
  // TODO(heshan): will use adapter instead after ios platfrom ready
  NativeFacadeDarwin(id<TemplateRenderCallbackProtocol> render)
      : _render(render) {}
  ~NativeFacadeDarwin() override = default;

  NativeFacadeDarwin(const NativeFacadeDarwin& facade) = delete;
  NativeFacadeDarwin& operator=(const NativeFacadeDarwin&) = delete;

  NativeFacadeDarwin(NativeFacadeDarwin&& facade) = default;
  NativeFacadeDarwin& operator=(NativeFacadeDarwin&&) = default;

  void OnDataUpdated() override;

  void OnPageChanged(bool is_first_screen) override;

  void OnTasmFinishByNative() override;

  void OnTemplateLoaded(const std::string& url) override;

  void OnRuntimeReady() override;

  void ReportError(const base::LynxError& error) override;

  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t code) override;

  void OnTimingSetup(const lepus::Value& timing_info) override;

  void OnTimingUpdate(const lepus::Value& timing_info,
                      const lepus::Value& update_timing,
                      const std::string& update_flag) override;

  void OnDynamicComponentPerfReady(const lepus::Value& perf_info) override;

  void OnConfigUpdated(const lepus::Value& data) override;

  void OnUpdateDataWithoutChange() override;

  void TriggerLepusMethodAsync(const std::string& method_name,
                               const lepus::Value& argus) override;

  void InvokeUIMethod(const tasm::LynxGetUIResult& ui_result,
                      const std::string& method,
                      fml::RefPtr<tasm::PropBundle> params,
                      piper::ApiCallBack callback) override;

  void FlushJSBTiming(piper::NativeModuleInfo timing) override;

  void OnSSRHydrateFinished(const std::string& url) override;

  void OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) override;

  virtual void OnReceiveMessageEvent(runtime::MessageEvent event) override;

  void OnEventCapture(long target_id, bool is_catch, int64_t event_id) override;

  void OnEventBubble(long target_id, bool is_catch, int64_t event_id) override;

  void OnEventFire(long target_id, bool is_stop, int64_t event_id) override;

  void OnLynxEvent(const lepus::Value& event_detail) override;

 private:
  __weak id<TemplateRenderCallbackProtocol> _render;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_IOS_NATIVE_FACADE_DARWIN_H_
