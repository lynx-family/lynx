// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_TEMPLATE_RENDERER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_TEMPLATE_RENDERER_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/value/base_value.h"
#include "core/public/devtool/lynx_devtool_proxy.h"
#include "core/public/devtool/lynx_inspector_owner.h"
#include "core/public/lynx_extension_delegate.h"
#include "core/public/ui_delegate.h"
#include "core/renderer/data/template_data.h"
#include "core/resource/lynx_resource_loader_harmony.h"
#include "core/runtime/bindings/jsi/modules/harmony/module_factory_harmony.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"
#include "core/services/performance/harmony/performance_controller_harmony.h"
#include "core/shell/lynx_shell.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "harmony/lynx_harmony/src/main/cpp/lynx_runtime_wrapper.h"

namespace lynx {

namespace piper {
class LynxModuleManager;
}  // namespace piper
namespace shell {
class LynxEngineProxy;
class LynxRuntimeProxy;
class LynxShell;
}  // namespace shell
namespace tasm {
class LynxTemplateBundle;
class TemplateData;
class PageConfig;
}  // namespace tasm

namespace harmony {

class LynxTemplateRenderer : public devtool::LynxDevToolProxy {
 public:
  LynxTemplateRenderer(napi_env env, napi_value js_this,
                       double display_density);

  virtual ~LynxTemplateRenderer();

  void LoadTemplate(
      const std::string& url, std::vector<uint8_t> source,
      const std::shared_ptr<lynx::tasm::PipelineOptions>& pipeline_options,
      const std::shared_ptr<lynx::tasm::TemplateData>& template_data,
      bool enable_recycle_template_bundle);
  void ReloadTemplate(
      const std::shared_ptr<tasm::TemplateData>& data,
      const std::shared_ptr<lynx::tasm::PipelineOptions>& pipeline_options,
      lepus::Value global_props);
  void LoadTemplateBundle(
      const std::string& url, const lynx::tasm::LynxTemplateBundle& bundle,
      const std::shared_ptr<lynx::tasm::PipelineOptions>& pipeline_options,
      const std::shared_ptr<lynx::tasm::TemplateData>& template_data,
      bool enable_dump_element_tree);
  void UpdateMetaData(const std::shared_ptr<tasm::TemplateData>& data,
                      lepus::Value global_props);
  void UpdateViewport(float width, int width_mode, float height,
                      int height_mode);
  void OnEnterForeground();
  void OnEnterBackground();
  void UpdateScreenMetrics(float width, float height, float display_density);
  void UpdateGlobalProps(lepus::Value value);
  lepus::Value GetAllTimingInfo() const;
  int32_t GetInstanceId() const;
  void UpdateFontScale(float font_scale);
  void SetEnableBytecode(bool enable, std::string source_url);
  lepus::Value GetPageDataByKey(std::vector<std::string> keys);

  void OnLoaded(const std::string& url);
  void OnRuntimeReady();
  void OnDataUpdated();
  void OnPageChanged(bool is_first_screen);
  void OnFirstLoadPerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing);
  void OnUpdatePerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing);
  void OnErrorOccurred(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only);
  void OnThemeUpdatedByJs(
      const std::unordered_map<std::string, std::string>& theme);
  void OnReloadTemplate(const std::string& url,
                        const std::vector<uint8_t>& source,
                        const std::shared_ptr<tasm::TemplateData>& data);
  void OnTemplateBundleReady(const tasm::LynxTemplateBundle& bundle);
  void OnPageConfigDecoded(const std::shared_ptr<tasm::PageConfig>& config);
  lepus::Value TriggerLepusMethod(const std::string& method_name,
                                  const lepus::Value& args);
  void TriggerLepusMethodAsync(const std::string& method_name,
                               const lepus::Value& args);

  // LynxEmbedderProxy
  void ReloadTemplate(const std::string& url,
                      const std::vector<uint8_t>& source,
                      const std::shared_ptr<tasm::TemplateData>& data) override;
  void LoadTemplateFromURL(
      const std::string& url,
      const std::shared_ptr<tasm::TemplateData> init_data = nullptr) override;
  double GetScreenScaleFactor() override;
  void TakeSnapshot(
      size_t max_width, size_t max_height, int quality,
      float screen_scale_factor,
      const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
      tasm::TakeSnapshotCompletedCallback callback) override;
  int GetNodeForLocation(int x, int y) override;
  std::vector<float> GetTransformValue(
      int id, const std::vector<float>& pad_border_margin_layout) override;
  void SetInspectorOwner(devtool::LynxInspectorOwner* owner) override;
  void EmulateTouch(const std::string& event_type, int x, int y,
                    const std::string& button, float delta_x, float delta_y,
                    int modifiers, int click_count) override;

  void SetUpLynxShell(
      napi_env env, tasm::UIDelegate* ui_delegate,
      const std::shared_ptr<LynxResourceLoaderHarmony>& resource_loader,
      float width, float height, bool is_host_renderer,
      tasm::performance::PerformanceControllerHarmonyJSWrapper*
          js_perf_controller_wrapper,
      int32_t thread_mode, std::string group_id, bool use_quickjs,
      bool enable_js_group_thread, std::vector<std::string> preload_js_paths,
      bool enable_bytecode, std::string bytecode_source_url, bool enable_js,
      std::unique_ptr<ModuleFactoryHarmony> module_factory,
      LynxRuntimeWrapper* runtime_wrapper);

  static napi_value Init(napi_env env, napi_value exports);
  static napi_value GetBaseTraceBackend(napi_env env, napi_callback_info info);
  static napi_value InitGlobalEnv(napi_env env, napi_callback_info info);
  static napi_value NativeAttach(napi_env env, napi_callback_info info);
  static napi_value NativeDetach(napi_env env, napi_callback_info info);
  static napi_value NativeReset(napi_env env, napi_callback_info info);
  static napi_value SetTracingDirPath(napi_env env, napi_callback_info info);
  static napi_value SetCacheDirPath(napi_env env, napi_callback_info info);
  static napi_value TraceEventBegin(napi_env env, napi_callback_info info);
  static napi_value TraceEventEnd(napi_env env, napi_callback_info info);
  static napi_value TraceInstant(napi_env env, napi_callback_info info);
  static napi_value ParserTestBenchRecordData(napi_env env,
                                              napi_callback_info info);
  static napi_value New(napi_env env, napi_callback_info info);
  static napi_value UpdateGlobalProps(napi_env env, napi_callback_info info);
  static napi_value UpdateMetaData(napi_env env, napi_callback_info info);
  static napi_value LoadTemplate(napi_env env, napi_callback_info info);
  static napi_value ReloadTemplate(napi_env env, napi_callback_info info);
  static napi_value LoadTemplateBundle(napi_env env, napi_callback_info info);
  static napi_value UpdateViewport(napi_env env, napi_callback_info info);
  static napi_value UpdateScreenMetrics(napi_env env, napi_callback_info info);
  static napi_value CallJSFunction(napi_env env, napi_callback_info info);
  static napi_value TriggerEventBus(napi_env env, napi_callback_info info);
  static napi_value CallJSApiCallbackWithValue(napi_env env,
                                               napi_callback_info info);
  static napi_value CallJSIntersectionObserver(napi_env env,
                                               napi_callback_info info);
  static napi_value EvaluateScript(napi_env env, napi_callback_info info);
  static napi_value RejectDynamicComponentLoad(napi_env env,
                                               napi_callback_info info);
  static napi_value SendTouchEvent(napi_env env, napi_callback_info info);
  static napi_value SendCustomEvent(napi_env env, napi_callback_info info);
  static napi_value ScrollByListContainer(napi_env env,
                                          napi_callback_info info);
  static napi_value ScrollToPosition(napi_env env, napi_callback_info info);
  static napi_value ScrollStopped(napi_env env, napi_callback_info info);
  static napi_value GetAllTimingInfo(napi_env env, napi_callback_info info);
  static napi_value GetInstanceId(napi_env env, napi_callback_info info);
  static napi_value UpdateFontScale(napi_env env, napi_callback_info info);
  static napi_value NativeSetEnableBytecode(napi_env env,
                                            napi_callback_info info);
  static napi_value GetPageDataByKey(napi_env env, napi_callback_info info);
  static napi_value SetupExtensionDelegate(napi_env env,
                                           napi_callback_info info);
  static napi_value OnEnterForeground(napi_env env, napi_callback_info info);
  static napi_value OnEnterBackground(napi_env env, napi_callback_info info);

  static napi_value GetAllJsSource(napi_env env, napi_callback_info info);
  static napi_value InvokeLepusCallback(napi_env env, napi_callback_info info);

  struct WeakFlag : public std::enable_shared_from_this<WeakFlag> {
    explicit WeakFlag(LynxTemplateRenderer* template_renderer)
        : renderer(template_renderer) {}
    LynxTemplateRenderer* renderer;
  };

 private:
  void MergeGlobalProps(lepus::Value global_props);
  void SetupExtensionDelegate(pub::LynxExtensionDelegate* delegate);
  std::vector<uint8_t> LoadJSSource(const std::string& url);
  std::shared_ptr<tasm::PipelineOptions> ProcessLoadTemplateTimingOption(
      napi_env env, napi_value arg, std::string pipeline_origin);
  void ResetTimingBeforeReload() const;

  napi_env env_;
  napi_ref template_renderer_ref_;
  lepus::Value global_props_;
  float display_density_{1.f};

  std::shared_ptr<shell::LynxEngineProxy> engine_proxy_;
  std::shared_ptr<shell::LynxRuntimeProxy> runtime_proxy_;
  std::shared_ptr<shell::LynxLayoutProxy> layout_proxy_;
  std::shared_ptr<shell::PerfControllerProxy> perf_controller_proxy_;
  std::shared_ptr<shell::LynxShell> shell_;
  // TODO(liyanbo.monster): use weak_ptr instead of shared_ptr
  std::shared_ptr<piper::LynxModuleManager> module_manager_;
  tasm::UIDelegate* ui_delegate_;
  std::shared_ptr<LynxResourceLoaderHarmony> resource_loader_;
  std::shared_ptr<WeakFlag> weak_flag_;
  devtool::LynxInspectorOwner* inspector_owner_ = nullptr;
};

}  // namespace harmony
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_TEMPLATE_RENDERER_H_
