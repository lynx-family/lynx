// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_CORE_LYNX_TEMPLATE_RENDERER_H_
#define PLATFORM_EMBEDDER_CORE_LYNX_TEMPLATE_RENDERER_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/public/devtool/lynx_devtool_proxy.h"
#include "core/public/devtool/lynx_inspector_owner.h"
#include "core/public/lynx_resource_loader.h"
#include "core/public/performance_controller_platform_impl.h"
#include "core/public/ui_delegate.h"
#include "core/renderer/data/template_data.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"
#include "core/shell/lynx_engine_proxy_impl.h"
#include "core/shell/lynx_runtime_proxy_impl.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/perf_controller_proxy_impl.h"
#if ENABLE_INSPECTOR
#include "devtool/embedder/common/devtools_embedder.h"
#endif  // ENABLE_INSPECTOR

namespace lynx {
namespace embedder {

struct LynxSize {
  float cx;
  float cy;
};

class TemplateRendererClient {
 public:
  virtual void OnLoaded(const std::string& url) {}
  virtual void OnRuntimeReady() {}
  virtual void OnDataUpdated() {}
  virtual void OnPageChanged(bool is_first_screen) {}
  virtual void OnFirstLoadPerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing) {}
  virtual void OnUpdatePerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing) {}
  virtual void OnErrorOccurred(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only) {}
  virtual void OnThemeUpdatedByJs(
      const std::unordered_map<std::string, std::string>& theme) {}
  virtual void OnLoadTemplate(const std::string& url,
                              const std::vector<uint8_t>& source,
                              const std::shared_ptr<tasm::TemplateData>& data) {
  }
  virtual void OnReloadTemplate(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<tasm::TemplateData>& data) {}
  virtual void OnLoadTemplateBundle(
      const std::string& url, const tasm::LynxTemplateBundle& template_bundle,
      const std::shared_ptr<tasm::TemplateData>& data) {}
  virtual void OnLoadScriptAsync(const std::string& url,
                                 const std::string& source) {}
  virtual void OnPageConfigDecoded(
      const std::shared_ptr<tasm::PageConfig>& config) {}

  virtual void OnTimingSetup(const lepus::Value& timing_info) {}
  virtual void OnTimingUpdate(const lepus::Value& timing_info,
                              const lepus::Value& update_timing,
                              const std::string& update_flag) {}
  virtual void OnPerformanceEvent(const lepus::Value& event_entry) {}
  virtual void OnTemplateBundleReady(const tasm::LynxTemplateBundle& bundle) {}
};

using TemplateVerification =
    std::function<bool(uint8_t* content, size_t length, const std::string url,
                       const char** error_msg)>;
using RuntimeProxyCallback =
    std::function<void(std::shared_ptr<shell::LynxEngineProxy>,
                       std::shared_ptr<shell::LynxRuntimeProxy>,
                       std::shared_ptr<piper::LynxModuleManager>,
                       const fml::RefPtr<fml::TaskRunner>&)>;

class LynxTemplateRenderer : public devtool::LynxDevToolProxy {
 public:
  struct Settings {
    std::shared_ptr<pub::LynxResourceLoader> resource_loader = nullptr;
    lepus::Value* global_props = nullptr;

    int thread_mode = 0;
    LynxSize screen_size;
    LynxSize viewport_size;
    float device_pixel_ratio = 1.f;
    bool enable_new_animator = false;
    bool enable_native_list = false;
    std::string group_id = "-1";
    bool enable_js_group_thread = false;
    std::vector<std::string> preload_js_paths;
    bool use_quickjs = true;
    bool enable_bytecode = false;
    std::string bytecode_source_url = "";
    bool enable_js = true;
  };

  LynxTemplateRenderer(
      const Settings& settings, tasm::UIDelegate* ui_delegate,
      std::shared_ptr<piper::LynxModuleManager> module_manager,
      std::unique_ptr<tasm::performance::PerformanceControllerPlatformImpl>
          perf_controller_ptr);
  LynxTemplateRenderer(
      const Settings& settings, tasm::UIDelegate* ui_delegate,
      std::shared_ptr<piper::LynxModuleManager> module_manager,
      std::unique_ptr<tasm::performance::PerformanceControllerPlatformImpl>
          perf_controller_ptr,
      RuntimeProxyCallback runtime_proxy_callback);
  virtual ~LynxTemplateRenderer();

  int32_t GetInstanceId();

  void Reset();

  void SetTemplateVerification(TemplateVerification verification) {
    template_verification_ = verification;
  }

  const Settings& GetSettings() const { return settings_; }

  void LoadTemplate(
      const std::string& url, std::vector<uint8_t> source,
      const std::shared_ptr<tasm::PipelineOptions>& pipeline_options,
      const std::shared_ptr<tasm::TemplateData>& data = nullptr,
      bool enable_recycle_template_bundle = false);
  void LoadTemplateBundle(
      const std::string& url, tasm::LynxTemplateBundle template_bundle,
      const std::shared_ptr<tasm::PipelineOptions>& pipeline_options,
      const std::shared_ptr<tasm::TemplateData>& data = nullptr,
      bool enable_dump_element_tree = false);
  void UpdateGlobalProps(const lepus::Value& global_props);

  void UpdateDataByParsedData(const std::shared_ptr<tasm::TemplateData>& data);
  void ResetDataByParsedData(const std::shared_ptr<tasm::TemplateData>& data);
  void ReloadTemplate(
      const std::shared_ptr<tasm::TemplateData>& data,
      const std::shared_ptr<tasm::PipelineOptions>& pipeline_options,
      const lepus::Value& global_props);
  void PreloadDynamicComponents(std::vector<std::string> urls);
  void RegisterDynamicComponent(std::string url,
                                tasm::LynxTemplateBundle template_bundle);
  void UpdateMetaData(const std::shared_ptr<tasm::TemplateData>& data,
                      const lepus::Value& global_props);

  void UpdateScreenMetrics(float width, float height, float scale);
  void UpdateViewport(float width, int width_mode, float height,
                      int height_mode, bool need_layout);
  void UpdateFontScale(float scale);
  void SetFontScale(float scale);

  void TriggerEventBus(const std::string& name, const lepus::Value& params);
  void SendGlobalEvent(const std::string& name, const lepus::Value& params);
  bool SendTouchEvent(const std::string& name, int32_t tag, float x, float y,
                      float client_x, float client_y, float page_x,
                      float page_y);

#if ENABLE_INSPECTOR
  void InvokeCDPFromSDK(const std::string& cdp_msg,
                        std::function<void(const std::string&)>&& callback);
#endif  // ENABLE_INSPECTOR

  void AddClient(TemplateRendererClient* client);
  void RemoveClient(TemplateRendererClient* client);

  bool IsDestroyed() const {
    return shell_ == nullptr || engine_proxy_ == nullptr;
  }

  void OnEnterForeground();
  void OnEnterBackground();

  // NativeFacade
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
  void OnPerformanceEvent(const lepus::Value& entry);
  void OnPageConfigDecoded(const std::shared_ptr<tasm::PageConfig>& config);
  void OnTimingSetup(const lepus::Value& timing_info);
  void OnTimingUpdate(const lepus::Value& timing_info,
                      const lepus::Value& update_timing,
                      const std::string& update_flag);

  void SetTiming(uint64_t us_timestamp, std::string timing_key,
                 std::string pipeline_id) const;
  const lepus::Value GetAllTimingInfo() const;

  // LynxEmbedderProxy
  void ReloadTemplate(const std::string& url,
                      const std::vector<uint8_t>& source,
                      const std::shared_ptr<tasm::TemplateData>& data) override;
  void LoadTemplateFromURL(
      const std::string& url,
      const std::shared_ptr<tasm::TemplateData> data = nullptr) override;
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

  std::shared_ptr<shell::LynxRuntimeProxy> GetRuntimeProxy() const;
  std::shared_ptr<piper::LynxModuleManager> GetModuleManager() const;

  struct WeakFlag : public std::enable_shared_from_this<WeakFlag> {
    explicit WeakFlag(LynxTemplateRenderer* template_renderer)
        : renderer(template_renderer) {}
    LynxTemplateRenderer* renderer;
  };

 protected:
  std::vector<uint8_t> LoadJSSource(const std::string& url);
  void UpdateGenericInfoWithUrl(const std::string& url);
  void ClearGenericInfo(int32_t instance_id);

  Settings settings_;
  std::shared_ptr<shell::LynxEngineProxy> engine_proxy_;
  std::shared_ptr<shell::LynxRuntimeProxy> runtime_proxy_;
  std::shared_ptr<shell::PerfControllerProxy> perf_controller_proxy_;
  std::unique_ptr<shell::LynxShell> shell_;
  tasm::UIDelegate* ui_delegate_;
  std::shared_ptr<WeakFlag> weak_flag_;
  std::shared_ptr<piper::LynxModuleManager> module_manager_;
  std::unique_ptr<tasm::performance::PerformanceControllerPlatformImpl>
      perf_controller_ptr_;
  RuntimeProxyCallback runtime_proxy_callback_;
  TemplateVerification template_verification_ = nullptr;

  std::list<TemplateRendererClient*> clients_;

#if ENABLE_INSPECTOR
  std::unique_ptr<devtool::DevtoolsEmbedder> devtools_;
#endif  // ENABLE_INSPECTOR
  devtool::LynxInspectorOwner* inspector_owner_ = nullptr;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CORE_LYNX_TEMPLATE_RENDERER_H_
