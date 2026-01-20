// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/core/lynx_template_renderer.h"

#include "core/runtime/lepus/json_parser.h"
#include "core/services/performance/performance_controller.h"
#include "core/shell/runtime/common/module_delegate_impl.h"
#include "platform/embedder/core/native_facade_impl.h"
#include "platform/embedder/core/performance/performance_controller_impl.h"
#include "platform/embedder/core/tasm_platform_invoker_impl.h"
#if ENABLE_INSPECTOR
#include "devtool/embedder/core/lynx_devtool_set_module.h"
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"
#endif
#include "core/services/event_report/event_tracker.h"
#include "core/services/timing_handler/timing_constants.h"

namespace lynx {
namespace embedder {

namespace {
void PrepareEnvWidthScreenSize(int width, int height, float density,
                               float ratio) {
  tasm::Config::InitializeVersion("1.0");
  tasm::Config::InitPixelValues(width * ratio, height * ratio, ratio);

  starlight::ComputedCSSStyle::SAFE_AREA_INSET_TOP_ = 0;
  starlight::ComputedCSSStyle::SAFE_AREA_INSET_BOTTOM_ = 0;
  starlight::ComputedCSSStyle::SAFE_AREA_INSET_LEFT_ = 0;
  starlight::ComputedCSSStyle::SAFE_AREA_INSET_RIGHT_ = 0;
}
}  // namespace

const std::string TemplateRendererEventSimulationProxy::kMousePressed =
    "mousePressed";
const std::string TemplateRendererEventSimulationProxy::kMouseMoved =
    "mouseMoved";
const std::string TemplateRendererEventSimulationProxy::kMouseReleased =
    "mouseReleased";
const std::string TemplateRendererEventSimulationProxy::kMouseWheel =
    "mouseWheel";
const std::string TemplateRendererEventSimulationProxy::kMouseLeftButton =
    "left";
const std::string TemplateRendererEventSimulationProxy::kMouseRightButton =
    "right";

LynxTemplateRenderer::LynxTemplateRenderer(
    const LynxTemplateRenderer::Settings& settings,
    tasm::UIDelegate* ui_delegate,
    std::shared_ptr<runtime::js::LynxModuleManager> module_manager,
    std::unique_ptr<tasm::performance::PerformanceControllerPlatformImpl>
        perf_controller_ptr)
    : LynxTemplateRenderer(settings, ui_delegate, module_manager,
                           std::move(perf_controller_ptr), nullptr) {}

LynxTemplateRenderer::LynxTemplateRenderer(
    const Settings& settings, tasm::UIDelegate* ui_delegate,
    std::shared_ptr<runtime::js::LynxModuleManager> module_manager,
    std::unique_ptr<tasm::performance::PerformanceControllerPlatformImpl>
        perf_controller_ptr,
    RuntimeProxyCallback runtime_proxy_callback)
    : settings_(settings),
      ui_delegate_(ui_delegate),
      weak_flag_(std::make_shared<WeakFlag>(this)),
      module_manager_(module_manager),
      perf_controller_ptr_(std::move(perf_controller_ptr)),
      runtime_proxy_callback_(runtime_proxy_callback) {
  if (!perf_controller_ptr_) {
    perf_controller_ptr_ = std::make_unique<PerformanceControllerImpl>(
        weak_flag_->weak_from_this());
  }
  Reset();
}

LynxTemplateRenderer::~LynxTemplateRenderer() {
  auto instance_id = GetInstanceId();
  shell_.reset();
  runtime_proxy_.reset();
  engine_proxy_.reset();
  perf_controller_proxy_.reset();
  ClearGenericInfo(instance_id);
}

int32_t LynxTemplateRenderer::GetInstanceId() {
  if (!shell_) {
    return shell::kUnknownInstanceId;
  }
  return shell_->GetInstanceId();
}

void LynxTemplateRenderer::Reset() {
  // If the screen size is physical, density equals the device pixel ratio and
  // the ratio should be 1. If the screen size is a logical size, density equals
  // 1, the ratio should be device pixel ratio.
  const float density =
      ui_delegate_->UsesLogicalPixels() ? 1 : settings_.device_pixel_ratio;
  const float ratio =
      ui_delegate_->UsesLogicalPixels() ? settings_.device_pixel_ratio : 1;
  auto screen_size = settings_.screen_size;
  PrepareEnvWidthScreenSize(screen_size.cx, screen_size.cy, density, ratio);

  auto view_size = settings_.viewport_size;
  auto lynx_env_config =
      tasm::LynxEnvConfig(view_size.cx, view_size.cy, density, ratio);
  auto instance_id = GetInstanceId();
  shell_.reset();
#if ENABLE_INSPECTOR
  auto& devtool_env = DevToolEnvEmbedder::GetInstance();
  bool is_devtool_enabled =
      devtool_env.IsLynxDebugEnabled() && devtool_env.IsDevToolEnabled();
#endif
  ClearGenericInfo(instance_id);
  auto native_facade = std::make_unique<NativeFacadeImpl>(this);
  auto loader =
      std::make_shared<tasm::LazyBundleLoader>(settings_.resource_loader);
  shell::ShellOption shell_option;
  shell_option.js_group_thread_name_ =
      settings_.enable_js_group_thread ? settings_.group_id : "";
  shell_option.enable_js_group_thread_ = settings_.enable_js_group_thread;
  shell_option.enable_js_ = settings_.enable_js;
  shell_.reset(
      shell::LynxShellBuilder()
          .SetNativeFacade(std::move(native_facade))
          .SetUseInvokeUIMethodFunction(true)
          .SetPaintingContextPlatformImpl(ui_delegate_->CreatePaintingContext())
          .SetLynxEnvConfig(lynx_env_config)
          .SetVSyncMonitorPlatformImpl(settings_.vsync_monitor_platform_impl)
          .SetEnableElementManagerVsyncMonitor(true)
          .SetEnableNewAnimator(settings_.enable_new_animator)
          .SetEnableNativeList(settings_.enable_native_list)
          .SetLazyBundleLoader(loader)
          .SetLayoutContextPlatformImpl(ui_delegate_->CreateLayoutContext())
          .SetStrategy(static_cast<base::ThreadStrategyForRendering>(
              settings_.thread_mode))
          .SetPropBundleCreator(ui_delegate_->CreatePropBundleCreator())
          .SetEngineActor(
              [loader](auto& actor) { loader->SetEngineActor(actor); })
          .SetShellOption(shell_option)
          .SetTasmPlatformInvoker(std::make_unique<TasmPlatformInvokerImpl>(
              weak_flag_->weak_from_this()))
          .SetPerformanceControllerPlatform(std::move(perf_controller_ptr_))
          .build());

  engine_proxy_ =
      std::make_shared<shell::LynxEngineProxyImpl>(shell_->GetEngineActor());
  perf_controller_proxy_ = std::make_shared<shell::PerfControllerProxyImpl>(
      shell_->GetPerfControllerActor());
  layout_proxy_ =
      std::make_shared<shell::LynxLayoutProxyImpl>(shell_->GetLayoutActor());

  // InitJSBridge
  if (!module_manager_) {
    module_manager_ = std::make_shared<runtime::js::LynxModuleManager>();
  }
  module_manager_->SetModuleFactory(ui_delegate_->GetCustomModuleFactory());
#if ENABLE_INSPECTOR
  if (is_devtool_enabled) {
    if (!devtools_) {
      devtools_ = std::make_unique<devtool::DevtoolsEmbedder>(this);
    }
    devtools_->GetInspectorOwner()->OnTemplateAssemblerCreated(
        reinterpret_cast<intptr_t>(shell_.get()));
  }
  std::unique_ptr<runtime::NativeModuleFactory> devtool_module_factory_ =
      std::make_unique<runtime::NativeModuleFactory>();
  devtool_module_factory_->Register(devtool::LynxDevToolSetModule::GetName(),
                                    devtool::LynxDevToolSetModule::Create);
  module_manager_->SetModuleFactory(std::move(devtool_module_factory_));
#endif
  auto on_runtime_actor_created = [this](auto& actor) {
    auto module_delegate = std::make_shared<shell::ModuleDelegateImpl>(
        shell_->GetRuntimeActor(), shell_->GetFacadeActor());
    module_manager_->initBindingPtr(module_manager_, module_delegate);
    runtime_proxy_ = std::make_shared<shell::LynxBTSRuntimeProxyImpl>(actor);
    module_manager_->runtime_proxy = runtime_proxy_;
    if (runtime_proxy_callback_) {
      // There's cases that need to inject runtime life cycle before JSRuntime
      // creating, so add an early callback here.
      // e.g. inject JSB `LynxRecorderReplayDataModule` for testbench.
      runtime_proxy_callback_(engine_proxy_, runtime_proxy_, module_manager_,
                              shell_->GetRunners()->GetJSTaskRunner());
    }
  };

  if (settings_.global_props) {
    shell_->UpdateGlobalProps(*settings_.global_props);
  }

  auto runtime_flags = shell::CalcRuntimeFlags(
      false, settings_.use_quickjs, false, settings_.enable_bytecode);
  shell_->InitRuntime(settings_.group_id, settings_.resource_loader,
                      module_manager_, std::move(on_runtime_actor_created),
                      std::move(settings_.preload_js_paths), runtime_flags,
                      settings_.bytecode_source_url,
                      settings_.vsync_monitor_platform_impl);

  ui_delegate_->OnLynxCreate(
      shell_->GetListEngineProxy(), engine_proxy_, runtime_proxy_,
      layout_proxy_, perf_controller_proxy_, settings_.resource_loader,
      shell_->GetRunners()->GetUITaskRunner(),
      shell_->GetRunners()->GetLayoutTaskRunner(), shell_->GetInstanceId(),
      shell_->GetPageOptions().IsEmbeddedModeOn());
}

void LynxTemplateRenderer::LoadTemplate(
    const std::string& url, std::vector<uint8_t> source,
    const std::shared_ptr<tasm::PipelineOptions>& pipeline_options,
    const std::shared_ptr<tasm::TemplateData>& init_data,
    bool enable_recycle_template_bundle) {
  if (source.empty()) {
    return;
  }
  if (template_verification_) {
    const char* error_msg = nullptr;
    if (!template_verification_(&source[0], source.size(), url, &error_msg)) {
      LOGE("LynxTemplateRenderer LoadTemplate verification failed");
      return;
    }
  }

  for (auto* client : clients_) {
    client->OnLoadTemplate(url, source, init_data);
  }
  if (inspector_owner_) {
    inspector_owner_->OnLoadTemplate(url, source, init_data);
    inspector_owner_->OnLoaded(url);
  }
  std::shared_ptr<tasm::PipelineOptions> options = nullptr;
  if (!pipeline_options) {
    options = std::make_shared<tasm::PipelineOptions>();
    options->need_timestamps = true;
    options->pipeline_origin = tasm::timing::kLoadBundle;
    shell_->OnPipelineStart(options->pipeline_id, options->pipeline_origin,
                            options->pipeline_start_timestamp);
  } else {
    options = pipeline_options;
  }

  UpdateGenericInfoWithUrl(url);
  if (perf_controller_proxy_) {
    perf_controller_proxy_->MarkTiming(
        tasm::timing::TimestampKey(tasm::timing::kLoadBundleStart),
        options->pipeline_id);
    perf_controller_proxy_->MarkTiming(
        tasm::timing::TimestampKey(tasm::timing::kFfiStart),
        options->pipeline_id);
  }

  options->enable_recycle_template_bundle = enable_recycle_template_bundle;
  options->enable_pre_painting = false;
  shell_->LoadTemplate(url, std::move(source), options, init_data);
}

void LynxTemplateRenderer::LoadTemplateBundle(
    const std::string& url, tasm::LynxTemplateBundle template_bundle,
    const std::shared_ptr<tasm::PipelineOptions>& pipeline_options,
    const std::shared_ptr<tasm::TemplateData>& init_data,
    bool enable_dump_element_tree) {
  for (auto* client : clients_) {
    client->OnLoadTemplateBundle(url, template_bundle, init_data);
  }

  std::shared_ptr<tasm::PipelineOptions> options = nullptr;
  if (!pipeline_options) {
    options = std::make_shared<tasm::PipelineOptions>();
    options->need_timestamps = true;
    options->pipeline_origin = tasm::timing::kLoadBundle;
    shell_->OnPipelineStart(options->pipeline_id, options->pipeline_origin,
                            options->pipeline_start_timestamp);
  } else {
    options = pipeline_options;
  }

  options->enable_pre_painting = false;
  options->enable_dump_element_tree = enable_dump_element_tree;
  shell_->LoadTemplateBundle(url, std::move(template_bundle), options,
                             init_data);
}

void LynxTemplateRenderer::UpdateGlobalProps(const lepus::Value& global_props) {
  shell_->UpdateGlobalProps(global_props);
}

void LynxTemplateRenderer::UpdateMetaData(
    const std::shared_ptr<tasm::TemplateData>& data,
    const lepus::Value& global_props) {
  shell_->UpdateMetaData(data, global_props);
}

void LynxTemplateRenderer::UpdateDataByParsedData(
    const std::shared_ptr<tasm::TemplateData>& data) {
  shell_->UpdateDataByParsedData(data);
}

void LynxTemplateRenderer::ResetDataByParsedData(
    const std::shared_ptr<tasm::TemplateData>& data) {
  shell_->ResetDataByParsedData(data);
}

void LynxTemplateRenderer::ReloadTemplate(
    const std::shared_ptr<tasm::TemplateData>& data,
    const std::shared_ptr<tasm::PipelineOptions>& pipeline_options,
    const lepus::Value& global_props) {
  shell_->ReloadTemplate(data, pipeline_options, global_props);
}

void LynxTemplateRenderer::PreloadDynamicComponents(
    std::vector<std::string> urls) {
  shell_->PreloadLazyBundles(urls);
}

void LynxTemplateRenderer::RegisterDynamicComponent(
    std::string url, tasm::LynxTemplateBundle template_bundle) {
  shell_->RegisterLazyBundle(url, template_bundle);
}

void LynxTemplateRenderer::UpdateScreenMetrics(float width, float height,
                                               float device_ratio) {
  DCHECK(device_ratio > 0.f);
  settings_.device_pixel_ratio = device_ratio;
  // The width/height is in pixels, convert to logical size if needed.
  if (ui_delegate_->UsesLogicalPixels()) {
    width = width / device_ratio;
    height = height / device_ratio;
  }
  settings_.screen_size.cx = width;
  settings_.screen_size.cy = height;
  float device_pixel_ratio =
      ui_delegate_->UsesLogicalPixels() ? device_ratio : 1.0f;
  ui_delegate_->OnUpdateScreenMetrics(width, height, device_pixel_ratio);
  tasm::Config::InitPixelValues(width, height, device_pixel_ratio);
  shell_->UpdateScreenMetrics(width, height, device_pixel_ratio);
}

void LynxTemplateRenderer::UpdateViewport(float width, int width_mode,
                                          float height, int height_mode,
                                          bool need_layout) {
  settings_.viewport_size.cx = width;
  settings_.viewport_size.cy = height;
  shell_->UpdateViewport(width, width_mode, height, height_mode, need_layout);
}

void LynxTemplateRenderer::SetFontScale(float scale) {
  shell_->SetFontScale(scale);
}

void LynxTemplateRenderer::UpdateFontScale(float scale) {
  shell_->UpdateFontScale(scale);
}

void LynxTemplateRenderer::TriggerEventBus(const std::string& name,
                                           const lepus::Value& params) {
  shell_->TriggerEventBus(name, params);
}

void LynxTemplateRenderer::SendGlobalEvent(const std::string& name,
                                           const lepus::Value& params) {
  if (!runtime_proxy_) {
    return;
  }
  auto args = lepus::CArray::Create();
  args->emplace_back(name);
  args->push_back(params);
  runtime_proxy_->CallJSFunction(
      "GlobalEventEmitter", "emit",
      std::make_unique<pub::ValueImplLepus>(lepus_value(args)));
}

bool LynxTemplateRenderer::SendTouchEvent(const std::string& name, int32_t tag,
                                          float x, float y, float client_x,
                                          float client_y, float page_x,
                                          float page_y) {
  return engine_proxy_->SendTouchEvent(name, tag, x, y, client_x, client_y,
                                       page_x, page_y);
}

#if ENABLE_INSPECTOR
void LynxTemplateRenderer::InvokeCDPFromSDK(
    const std::string& cdp_msg,
    std::function<void(const std::string&)>&& callback) {
  if (devtools_) {
    devtools_->GetInspectorOwner()->InvokeCDPFromSDK(cdp_msg,
                                                     std::move(callback));
  }
}
#endif  // ENABLE_INSPECTOR

void LynxTemplateRenderer::AddClient(TemplateRendererClient* client) {
  if (std::find(clients_.begin(), clients_.end(), client) == clients_.end()) {
    clients_.emplace_back(client);
  }
}

void LynxTemplateRenderer::RemoveClient(TemplateRendererClient* client) {
  clients_.erase(std::remove(clients_.begin(), clients_.end(), client),
                 clients_.end());
}

void LynxTemplateRenderer::OnEnterForeground() { shell_->OnEnterForeground(); }

void LynxTemplateRenderer::OnEnterBackground() { shell_->OnEnterBackground(); }

void LynxTemplateRenderer::OnLoaded(const std::string& url) {
  for (auto* client : clients_) {
    client->OnLoaded(url);
  }
}

void LynxTemplateRenderer::OnRuntimeReady() {
  for (auto* client : clients_) {
    client->OnRuntimeReady();
  }
}

void LynxTemplateRenderer::OnDataUpdated() {
  for (auto* client : clients_) {
    client->OnDataUpdated();
  }
}

void LynxTemplateRenderer::OnPageChanged(bool is_first_page) {
  for (auto* client : clients_) {
    client->OnPageChanged(is_first_page);
  }
}

void LynxTemplateRenderer::OnFirstLoadPerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  for (auto* client : clients_) {
    client->OnFirstLoadPerfReady(perf, perf_timing);
  }
}

void LynxTemplateRenderer::OnUpdatePerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  for (auto* client : clients_) {
    client->OnUpdatePerfReady(perf, perf_timing);
  }
}

void LynxTemplateRenderer::OnErrorOccurred(
    int level, int32_t error_code, const std::string& message,
    const std::string& fix_suggestion,
    const std::unordered_map<std::string, std::string>& custom_info,
    bool is_logbox_only) {
  for (auto* client : clients_) {
    client->OnErrorOccurred(level, error_code, message, fix_suggestion,
                            custom_info, is_logbox_only);
  }
}

void LynxTemplateRenderer::OnThemeUpdatedByJs(
    const std::unordered_map<std::string, std::string>& theme) {
  for (auto* client : clients_) {
    client->OnThemeUpdatedByJs(theme);
  }
}

void LynxTemplateRenderer::OnTemplateBundleReady(
    const tasm::LynxTemplateBundle& bundle) {
  for (auto* client : clients_) {
    client->OnTemplateBundleReady(bundle);
  }
}

void LynxTemplateRenderer::OnPerformanceEvent(const lepus::Value& entry) {
  for (auto* client : clients_) {
    client->OnPerformanceEvent(entry);
  }
}

void LynxTemplateRenderer::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  // Main thread
  for (auto* client : clients_) {
    client->OnPageConfigDecoded(config);
  }
  ui_delegate_->OnPageConfigDecoded(config);
}

void LynxTemplateRenderer::OnTimingSetup(const lepus::Value& timing_info) {
  for (auto* client : clients_) {
    client->OnTimingSetup(timing_info);
  }
}

void LynxTemplateRenderer::OnTimingUpdate(const lepus::Value& timing_info,
                                          const lepus::Value& update_timing,
                                          const std::string& update_flag) {
  for (auto* client : clients_) {
    client->OnTimingUpdate(timing_info, update_timing, update_flag);
  }
}

void LynxTemplateRenderer::SetTiming(uint64_t us_timestamp,
                                     std::string timing_key,
                                     std::string pipeline_id) const {
  return shell_->SetTiming(us_timestamp, std::move(timing_key),
                           std::move(pipeline_id));
}

const lepus::Value LynxTemplateRenderer::GetAllTimingInfo() const {
  return shell_->GetAllTimingInfo();
}

// TBD: LynxEmbedderProxy
void LynxTemplateRenderer::ReloadTemplate(
    const std::string& url, const std::vector<uint8_t>& source,
    const std::shared_ptr<tasm::TemplateData>& data) {
  for (auto* client : clients_) {
    client->OnReloadTemplate(url, source, data);
  }
}

void LynxTemplateRenderer::LoadTemplateFromURL(
    const std::string& url,
    const std::shared_ptr<tasm::TemplateData> init_data) {
  static const char* FILE_SCHEME = "file://";
  static const char* ASSETS_SCHEME = "assets://";
  std::shared_ptr<lynx::tasm::PipelineOptions> pipeline_options = nullptr;
  if (memcmp(url.c_str(), FILE_SCHEME, strlen(FILE_SCHEME)) == 0 ||
      memcmp(url.c_str(), ASSETS_SCHEME, strlen(ASSETS_SCHEME)) == 0) {
    LoadTemplate(url, LoadJSSource(url), pipeline_options, init_data);
    return;
  }

  auto resource_loader = settings_.resource_loader;
  if (!resource_loader) {
    return;
  }

  pub::LynxResourceRequest req{.url = url,
                               .type = pub::LynxResourceType::kTemplate};
  resource_loader->LoadResource(
      req, [weak_flag = weak_flag_->weak_from_this(),
            task_runner = shell_->GetRunners()->GetUITaskRunner(), url,
            init_data, pipeline_options](pub::LynxResourceResponse& response) {
        auto data_size = response.data.size();
        LOGI("LynxTemplateRenderer LoadTemplateFromURL data_size: "
             << data_size);
        fml::TaskRunner::RunNowOrPostTask(
            task_runner, [weak_flag, data = std::move(response.data),
                          url = std::move(url), init_data, pipeline_options] {
              if (auto self = weak_flag.lock()) {
                self->renderer->LoadTemplate(std::move(url), std::move(data),
                                             pipeline_options, init_data);
              }
            });
      });
}

double LynxTemplateRenderer::GetScreenScaleFactor() {
  return ui_delegate_->GetScreenScaleFactor();
}

void LynxTemplateRenderer::TakeSnapshot(
    size_t max_width, size_t max_height, int quality, float screen_scale_factor,
    const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
    tasm::TakeSnapshotCompletedCallback callback) {
  ui_delegate_->TakeSnapshot(max_width, max_height, quality,
                             screen_scale_factor, screenshot_runner,
                             std::move(callback));
}

int LynxTemplateRenderer::GetNodeForLocation(int x, int y) {
  return ui_delegate_->GetNodeForLocation(x, y);
}

std::vector<float> LynxTemplateRenderer::GetTransformValue(
    int id, const std::vector<float>& pad_border_margin_layout) {
  return ui_delegate_->GetTransformValue(id, pad_border_margin_layout);
}
void LynxTemplateRenderer::SetInspectorOwner(
    devtool::LynxInspectorOwner* owner) {
  inspector_owner_ = owner;
}

void LynxTemplateRenderer::EmulateTouch(const std::string& event_type, int x,
                                        int y, const std::string& button,
                                        float delta_x, float delta_y,
                                        int modifiers, int click_count) {
  if (event_proxy_) {
    event_proxy_->EmulateTouch(event_type, x, y, button, delta_x, delta_y,
                               modifiers, click_count);
  }
}

std::shared_ptr<shell::LynxRuntimeProxy> LynxTemplateRenderer::GetRuntimeProxy()
    const {
  return runtime_proxy_;
}

std::shared_ptr<runtime::js::LynxModuleManager>
LynxTemplateRenderer::GetModuleManager() const {
  return module_manager_;
}

std::vector<uint8_t> LynxTemplateRenderer::LoadJSSource(
    const std::string& url) {
  auto resource_loader = settings_.resource_loader;
  if (!resource_loader) {
    return std::vector<uint8_t>();
  }
  std::promise<std::vector<uint8_t>> promise;
  std::future<std::vector<uint8_t>> future = promise.get_future();
  auto request = pub::LynxResourceRequest{
      .url = url, .type = pub::LynxResourceType::kAssets};
  resource_loader->LoadResource(
      request, [promise = std::move(promise)](
                   pub::LynxResourceResponse& response) mutable {
        promise.set_value(std::move(response.data));
      });
  return future.get();
}

void LynxTemplateRenderer::UpdateGenericInfoWithUrl(const std::string& url) {
  auto instance_id = GetInstanceId();
  std::unordered_map<std::string, std::string> generic_infos{
      {tasm::report::kPropThreadMode, std::to_string(settings_.thread_mode)},
      {tasm::report::kPropURL, url}};
  // not support thread mode change in pc now
  tasm::report::EventTracker::UpdateGenericInfo(instance_id,
                                                std::move(generic_infos));
}

void LynxTemplateRenderer::ClearGenericInfo(int32_t instance_id) {
  if (instance_id != shell::kUnknownInstanceId) {
    tasm::report::EventTracker::ClearCache(instance_id);
  }
}

}  // namespace embedder
}  // namespace lynx
