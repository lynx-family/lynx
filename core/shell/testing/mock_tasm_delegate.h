// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TESTING_MOCK_TASM_DELEGATE_H_
#define CORE_SHELL_TESTING_MOCK_TASM_DELEGATE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/debug/lynx_error.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/jsi/api_call_back.h"
#include "core/shell/common/platform_call_back_manager.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

namespace lynx {
namespace tasm {
namespace test {

class MockTasmDelegate : public TemplateAssembler::Delegate,
                         public ElementManager::Delegate {
 public:
  MockTasmDelegate() = default;
  virtual ~MockTasmDelegate() {}
  virtual void OnDataUpdated() override;
  virtual void OnTasmFinishByNative() override;
  virtual void OnTemplateLoaded(const std::string& url) override;
  virtual void OnSSRHydrateFinished(const std::string& url) override;
  virtual void OnErrorOccurred(base::LynxError error) override;
  virtual void TriggerLepusngGc(base::closure func) override;
  virtual void OnDynamicComponentPerfReady(
      const lepus::Value& perf_info) override;
  virtual void OnConfigUpdated(const lepus::Value& data) override;
  virtual void OnPageConfigDecoded(
      const std::shared_ptr<tasm::PageConfig>& config) override;

  void RecycleTemplateBundle(
      std::unique_ptr<tasm::LynxBinaryRecyclerDelegate> recycler) override;

  tasm::LynxTemplateBundle GetTemplateBundle() { return std::move(bundle_); }

  // synchronous
  virtual std::string TranslateResourceForTheme(
      const std::string& res_id, const std::string& theme_key) override;

  virtual void GetI18nResource(const std::string& channel,
                               const std::string& fallback_url) override;

  virtual void OnI18nResourceChanged(const std::string& res) override;

  virtual void OnJSSourcePrepared(
      tasm::TasmRuntimeBundle bundle, const lepus::Value& global_props,
      const std::string& page_name, tasm::PackageInstanceDSL dsl,
      tasm::PackageInstanceBundleModuleMode bundle_module_mode,
      const std::string& url,
      const tasm::PipelineOptions& pipeline_options) override;
  void OnGlobalPropsUpdated(const lepus::Value& props) override;
  virtual void CallJSApiCallback(piper::ApiCallBack callback) override;
  virtual void CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                          const lepus::Value& value,
                                          bool persist = false) override;
  virtual void RemoveJSApiCallback(piper::ApiCallBack callback) override;

  void CallPlatformCallbackWithValue(
      const std::shared_ptr<shell::PlatformCallBackHolder>& callback,
      const lepus::Value& value) override;

  void RemovePlatformCallback(
      const std::shared_ptr<shell::PlatformCallBackHolder>& callback) override;

  virtual void CallJSFunction(
      const std::string& module_id, const std::string& method_id,
      const lepus::Value& arguments,
      bool force_call_despite_app_state = false) override;

  virtual void OnDataUpdatedByNative(tasm::TemplateData data,
                                     const bool reset = false) override;
  virtual void OnJSAppReload(
      tasm::TemplateData init_data,
      const tasm::PipelineOptions& pipeline_options) override;
  virtual void OnLifecycleEvent(const lepus::Value& args) override;
  virtual void PrintMsgToJS(const std::string& level,
                            const std::string& msg) override;

  virtual void SendAnimationEvent(const char* type, int tag,
                                  const lepus::Value& dict) override;
  virtual void SendNativeCustomEvent(const std::string& name, int tag,
                                     const lepus::Value& param_value,
                                     const std::string& param_name) override;

  const char* GetAnimationEventType() { return animation_event_type_; }
  void ClearAnimationEvent();
  const lepus::Value& GetAnimationEventParams() {
    return animation_event_params_;
  }

  // LynxEngine::Delegate
  void OnComponentDecoded(tasm::TasmRuntimeBundle bundle) override;

  void OnCardConfigDataChanged(const lepus::Value& data) override;

  fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() override {
    return nullptr;
  }

  void RequestVsync(
      uintptr_t id,
      base::MoveOnlyClosure<void, int64_t, int64_t> callback) override;

  lepus::Value TriggerLepusMethod(const std::string& method_id,
                                  const lepus::Value& arguments) override;

  void TriggerLepusMethodAsync(const std::string& method_id,
                               const lepus::Value& arguments,
                               bool is_air) override;

  std::string DumpDelegate() { return ss_.str(); }
  void ResetThemeConfig();

  void DispatchLayoutUpdates(const PipelineOptions& options) override {
    dispatch_layout_updates_called_ = true;
  }

  void SetEnableLayout() override { set_enable_layout_called_ = true; }

  MOCK_METHOD(void, UpdateLayoutNodeFontSize,
              (int32_t id, double cur_node_font_size,
               double root_node_font_size, double font_scale),
              (override));

  MOCK_METHOD(void, InsertLayoutNode,
              (int32_t parent_id, int32_t child_id, int index), (override));

  MOCK_METHOD(void, RemoveLayoutNodeAtIndex, (int32_t parent_id, int index),
              (override));

  MOCK_METHOD(void, InsertLayoutNodeBefore,
              (int32_t parent_id, int32_t child_id, int32_t ref_id),
              (override));

  MOCK_METHOD(void, RemoveLayoutNode, (int32_t parent_id, int32_t child_id),
              (override));

  MOCK_METHOD(void, DestroyLayoutNode, (int32_t id), (override));

  MOCK_METHOD(void, MoveLayoutNode,
              (int32_t parent_id, int32_t child_id, int from_index,
               int to_index),
              (override));

  MOCK_METHOD(void, UpdateLayoutNodeStyle,
              (int32_t id, tasm::CSSPropertyID css_id,
               const tasm::CSSValue& value),
              (override));

  MOCK_METHOD(void, ResetLayoutNodeStyle,
              (int32_t id, tasm::CSSPropertyID css_id), (override));

  MOCK_METHOD(void, UpdateLayoutNodeAttribute,
              (int32_t id, starlight::LayoutAttribute key,
               const lepus::Value& value),
              (override));

  void SetFontFaces(const CSSFontFaceRuleMap& fontfaces) override {}

  MOCK_METHOD(void, UpdateLayoutNodeByBundle,
              (int32_t id, std::unique_ptr<LayoutBundle> bundle), (override));
  MOCK_METHOD(void, UpdateLayoutNodeProps,
              (int32_t id, const std::shared_ptr<tasm::PropBundle>& props),
              (override));

  MOCK_METHOD(void, MarkLayoutDirty, (int32_t id), (override));

  MOCK_METHOD(void, UpdateLynxEnvForLayoutThread, (LynxEnvConfig env),
              (override));

  void OnUpdateViewport(float width, int width_mode, float height,
                        int height_mode, bool need_layout) override {}

  MOCK_METHOD(void, SetRootOnLayout, (int32_t id), (override));

  void OnUpdateDataWithoutChange() override {}

  void SetPageConfigForLayoutThread(
      const std::shared_ptr<PageConfig>& config) override {}

  MOCK_METHOD(void, CreateLayoutNode, (int32_t id, const base::String& tag),
              (override));

  MOCK_METHOD(void, AttachLayoutNodeType,
              (int32_t id, const base::String& tag, bool allow_inline,
               const std::shared_ptr<tasm::PropBundle>& props),
              (override));

  MOCK_METHOD((std::unordered_map<int32_t, tasm::LayoutInfoArray>),
              GetSubTreeLayoutInfo, (int32_t root_id, Viewport Viewport),
              (override));

#if ENABLE_TESTBENCH_RECORDER
  void SetRecordId(int64_t record_id) override {}
#endif
  void SetTiming(tasm::Timing timing) override {}
  void ResetTimingBeforeReload(const std::string& flag) override {}
  virtual void BindPipelineIDWithTimingFlag(
      const tasm::PipelineID& pipeline_id,
      const tasm::timing::TimingFlag& timing_flag) override{};
  virtual void OnPipelineStart(
      const tasm::PipelineID& pipeline_id,
      const tasm::PipelineOrigin& pipeline_origin,
      tasm::timing::TimestampUs pipeline_start_timestamp){};
  void InvokeUIMethod(tasm::LynxGetUIResult ui_result,
                      const std::string& method,
                      std::unique_ptr<tasm::PropBundle> params,
                      piper::ApiCallBack callback) override {}

  void LepusInvokeUIMethod(
      std::vector<int32_t> ui_impl_ids, const std::string& method,
      const lepus::Value& params, lepus::Context* context,
      std::unique_ptr<lepus::Value> callback_closure) override{};

  event::DispatchEventResult DispatchMessageEvent(
      runtime::MessageEvent event) override;

  bool has_received_animation_start_event() {
    return animation_start_event_count_ == 1;
  }
  bool has_received_animation_end_event() {
    return animation_end_event_count_ == 1;
  }
  bool has_received_animation_cancel_event() {
    return animation_cancel_event_count_ == 1;
  }
  bool has_received_animation_iteration_event() {
    return animation_iteration_event_count_ == 1;
  }
  bool only_received_animation_start_event() {
    return animation_start_event_count_ == 1 &&
           animation_end_event_count_ == 0 &&
           animation_cancel_event_count_ == 0 &&
           animation_iteration_event_count_ == 0;
  }
  bool only_received_animation_end_event() {
    return animation_start_event_count_ == 0 &&
           animation_end_event_count_ == 1 &&
           animation_cancel_event_count_ == 0 &&
           animation_iteration_event_count_ == 0;
  }
  bool only_received_animation_cancel_event() {
    return animation_start_event_count_ == 0 &&
           animation_end_event_count_ == 0 &&
           animation_cancel_event_count_ == 1 &&
           animation_iteration_event_count_ == 0;
  }
  bool only_received_animation_iteration_event() {
    return animation_start_event_count_ == 0 &&
           animation_end_event_count_ == 0 &&
           animation_cancel_event_count_ == 0 &&
           animation_iteration_event_count_ == 1;
  }
  bool not_received_any_event() {
    return animation_start_event_count_ == 0 &&
           animation_end_event_count_ == 0 &&
           animation_cancel_event_count_ == 0 &&
           animation_iteration_event_count_ == 0;
  }

 private:
  std::stringstream ss_;
  std::unique_ptr<std::unordered_map<std::string, std::string>>
      light_color_map_;
  std::unique_ptr<std::unordered_map<std::string, std::string>> dark_color_map_;
  std::unique_ptr<std::unordered_map<std::string, std::string>> theme_config_;
  void UpdateMockDelegateThemeConfig(const lepus::Value& data);

  bool set_enable_layout_called_{false};
  bool dispatch_layout_updates_called_{false};

  std::string lepus_method_id_{};
  lepus::Value lepus_method_arguments_{};

  // using for animation event test
  const char* animation_event_type_;
  lepus::Value animation_event_params_;
  size_t animation_start_event_count_{0};
  size_t animation_end_event_count_{0};
  size_t animation_cancel_event_count_{0};
  size_t animation_iteration_event_count_{0};

  // for recycle template bundle
  tasm::LynxTemplateBundle bundle_;
};

}  // namespace test

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_SHELL_TESTING_MOCK_TASM_DELEGATE_H_
