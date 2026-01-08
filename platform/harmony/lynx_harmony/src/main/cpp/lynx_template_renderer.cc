// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"

#include <js_native_api.h>

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/trace/native/platform/harmony/trace_controller_delegate_harmony.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/renderer/data/harmony/template_data_harmony.h"
#include "core/renderer/dom/harmony/lynx_template_bundle_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/ui_delegate_harmony.h"
#include "core/runtime/jscache/harmony/js_cache_manager_harmony.h"
#include "core/services/event_report/harmony/event_tracker_harmony.h"
#include "core/services/performance/harmony/performance_controller_harmony.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/shell/harmony/native_facade_harmony.h"
#include "core/shell/harmony/tasm_platform_invoker_harmony.h"
#include "core/shell/lynx_engine_proxy_impl.h"
#include "core/shell/lynx_layout_proxy_impl.h"
#include "core/shell/lynx_runtime_proxy_impl.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/module_delegate_impl.h"
#include "core/shell/perf_controller_proxy_impl.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/base/base_trace_backend.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/font/system_font_manager.h"

#if ENABLE_TESTBENCH_REPLAY
#include "core/services/replay/testbench_utils_embedder.h"
#endif

namespace lynx {
namespace harmony {

namespace {

// Check napi_unwrap object, print error message if failed.
// Do not throw if failed because we have not defined the exceptions for js api.
bool CheckNapiUnwrapObject(napi_status status, void* obj, const char* message) {
  if (status != napi_ok) {
    LOGE("napi unwrap status error: " << base::NapiUtil::StatusToString(status)
                                      << ", " << message);
    return false;
  }
  if (!obj) {
    LOGE("napi unwrap object is null: " << message);
    return false;
  }
  return true;
}

void PrepareEnvWidthScreenSize(int width, int height, float density,
                               float ratio, float display_density) {
  tasm::Config::InitializeVersion("1.0");
  tasm::Config::InitPixelValues(width * ratio, height * ratio, display_density);
  starlight::ComputedCSSStyle::SAFE_AREA_INSET_TOP_ = 0;
  starlight::ComputedCSSStyle::SAFE_AREA_INSET_BOTTOM_ = 0;
  starlight::ComputedCSSStyle::SAFE_AREA_INSET_LEFT_ = 0;
  starlight::ComputedCSSStyle::SAFE_AREA_INSET_RIGHT_ = 0;
}

}  // namespace

LynxTemplateRenderer::LynxTemplateRenderer(napi_env env, napi_value js_this,
                                           double display_density)
    : env_(env),
      display_density_(display_density),
      weak_flag_(std::make_shared<WeakFlag>(this)) {
  napi_create_reference(env, js_this, 0, &template_renderer_ref_);

  napi_value param[1];
  param[0] =
      base::NapiUtil::CreatePtrArray(env, reinterpret_cast<uintptr_t>(this));
  base::NapiUtil::InvokeJsMethod(env_, template_renderer_ref_, "createDevTool",
                                 1, param, nullptr);
}

LynxTemplateRenderer::~LynxTemplateRenderer() {
  LOGI("~TemplateRenderer");
  int32_t instance_id = GetInstanceId();
  if (resource_loader_) {
    static_cast<LynxResourceLoaderHarmony*>(resource_loader_.get())
        ->DeleteRef();
  }
  napi_delete_reference(env_, template_renderer_ref_);
  tasm::report::EventTracker::ClearCache(instance_id);

  SetInspectorOwner(nullptr);
}

void LynxTemplateRenderer::SetUpLynxShell(
    napi_env env, tasm::UIDelegate* ui_delegate,
    const std::shared_ptr<LynxResourceLoaderHarmony>& resource_loader,
    float width, float height, bool is_host_renderer,
    tasm::performance::PerformanceControllerHarmonyJSWrapper*
        js_perf_controller_wrapper,
    int32_t thread_mode, std::string group_id, bool use_quickjs,
    bool enable_js_group_thread, std::vector<std::string> preload_js_paths,
    bool enable_bytecode, std::string bytecode_source_url, bool enable_js,
    std::unique_ptr<ModuleFactoryHarmony> module_factory,
    LynxRuntimeWrapper* runtime_wrapper) {
  ui_delegate_ = ui_delegate;
  resource_loader_ = resource_loader;

  float w = width / display_density_;
  float h = height / display_density_;

  // If the screen size is physical, density equals the device pixel ratio and
  // the ratio should be 1. If the screen size is a logical size, density equals
  // 1, the ratio should be device pixel ratio.
  const float density = ui_delegate->UsesLogicalPixels() ? 1 : display_density_;
  const float ratio = ui_delegate->UsesLogicalPixels() ? display_density_ : 1;
  PrepareEnvWidthScreenSize(w, h, density, ratio, display_density_);
  auto lynx_env_config = tasm::LynxEnvConfig(w, h, density, ratio);
  auto loader = std::make_shared<tasm::LazyBundleLoader>(resource_loader);

  shell::ShellOption shell_option;
  shell_option.js_group_thread_name_ = enable_js_group_thread ? group_id : "";
  shell_option.enable_js_group_thread_ = enable_js_group_thread;
  shell_option.enable_js_ = enable_js;
  shell_option.instance_id_ =
      runtime_wrapper ? runtime_wrapper->RuntimeStandalone().GetRuntimeId()
                      : -1;

  auto invoker = std::make_unique<TasmPlatformInvokerHarmony>(
      weak_flag_->weak_from_this());
  auto* invoker_ptr = invoker.get();
  shell_.reset(
      shell::LynxShellBuilder()
          .SetNativeFacade(std::make_unique<NativeFacadeHarmony>(this))
          .SetUseInvokeUIMethodFunction(true)
          .SetPaintingContextPlatformImpl(ui_delegate_->CreatePaintingContext())
          .SetLynxEnvConfig(lynx_env_config)
          .SetEnableElementManagerVsyncMonitor(true)
          .SetEnableNewAnimator(is_host_renderer)
          .SetEnableNativeList(is_host_renderer)
          .SetLazyBundleLoader(loader)
          //.SetEnablePreUpdateData(enable_pre_update_data_)
          .SetLayoutContextPlatformImpl(ui_delegate_->CreateLayoutContext())
          .SetStrategy(
              static_cast<base::ThreadStrategyForRendering>(thread_mode))
          .SetPropBundleCreator(ui_delegate_->CreatePropBundleCreator())
          .SetEngineActor(
              [loader](auto& actor) { loader->SetEngineActor(actor); })
          .SetShellOption(shell_option)
          .SetTasmPlatformInvoker(std::move(invoker))
          .SetPerformanceControllerPlatform(
              std::make_unique<tasm::performance::PerformanceControllerHarmony>(
                  std::shared_ptr<
                      tasm::performance::PerformanceControllerHarmonyJSWrapper>(
                      js_perf_controller_wrapper)))
          .SetRuntimeActor(
              (runtime_wrapper != nullptr)
                  ? runtime_wrapper->RuntimeStandalone().GetRuntimeActor()
                  : nullptr)
          .SetPerfControllerActor((runtime_wrapper != nullptr)
                                      ? runtime_wrapper->RuntimeStandalone()
                                            .GetPerfControllerActor()
                                      : nullptr)
          .build());
  invoker_ptr->SetUITaskRunner(shell_->GetRunners()->GetUITaskRunner());

  if (inspector_owner_ != nullptr) {
    inspector_owner_->SetUITaskRunner(shell_->GetRunners()->GetUITaskRunner());
    inspector_owner_->OnTemplateAssemblerCreated(
        reinterpret_cast<intptr_t>(shell_.get()));
  }
  engine_proxy_ =
      std::make_shared<shell::LynxEngineProxyImpl>(shell_->GetEngineActor());
  if (runtime_wrapper) {
    module_manager_ = runtime_wrapper->GetModuleManager();
    module_manager_->SetModuleFactory(ui_delegate_->GetCustomModuleFactory());
    runtime_wrapper->SetAttached(true);
    shell_->AttachRuntime();
    runtime_proxy_ = runtime_wrapper->GetRuntimeProxy();
  } else {
    // InitJSBridge
    module_manager_ = std::make_shared<piper::LynxModuleManager>();
    module_manager_->SetModuleFactory(ui_delegate_->GetCustomModuleFactory());
    module_manager_->SetModuleFactory(std::move(module_factory));
    napi_value module_param[1];
    module_param[0] = base::NapiUtil::CreatePtrArray(
        env, reinterpret_cast<uintptr_t>(module_manager_.get()));
    base::NapiUtil::InvokeJsMethod(env_, template_renderer_ref_,
                                   "initNativeSetModule", 1, module_param,
                                   nullptr);

    auto on_runtime_actor_created = [this](auto& actor) {
      auto module_delegate = std::make_shared<shell::ModuleDelegateImpl>(
          shell_->GetRuntimeActor(), shell_->GetFacadeActor());
      module_manager_->initBindingPtr(module_manager_, module_delegate);
      runtime_proxy_ = std::make_shared<shell::LynxRuntimeProxyImpl>(actor);
      module_manager_->runtime_proxy = runtime_proxy_;
    };

    auto runtime_flags =
        runtime::CalcRuntimeFlags(false, use_quickjs, false, enable_bytecode);
    shell_->InitRuntime(group_id, resource_loader_, module_manager_,
                        std::move(on_runtime_actor_created),
                        std::move(preload_js_paths), runtime_flags,
                        bytecode_source_url);
  }

  perf_controller_proxy_ = std::make_shared<shell::PerfControllerProxyImpl>(
      shell_->GetPerfControllerActor());
  layout_proxy_ =
      std::make_shared<shell::LynxLayoutProxyImpl>(shell_->GetLayoutActor());
  ui_delegate_->OnLynxCreate(
      shell_->GetListEngineProxy(), engine_proxy_, runtime_proxy_,
      layout_proxy_, perf_controller_proxy_, resource_loader,
      shell_->GetRunners()->GetUITaskRunner(),
      shell_->GetRunners()->GetLayoutTaskRunner(), shell_->GetInstanceId(),
      shell_->GetPageOptions().IsEmbeddedModeOn());
  tasm::report::EventTracker::UpdateGenericInfo(
      GetInstanceId(), tasm::report::harmony::kPropThreadMode,
      static_cast<int64_t>(thread_mode));

  ui_delegate_->SetInstanceId(GetInstanceId());
}

void LynxTemplateRenderer::UpdateViewport(float width, int width_mode,
                                          float height, int height_mode) {
  if (shell_->IsDestroyed()) {
    return;
  }
  shell_->UpdateViewport(width, width_mode, height, height_mode, true);
}

void LynxTemplateRenderer::OnEnterForeground() {
  if (shell_->IsDestroyed()) {
    return;
  }
  shell_->OnEnterForeground();
}

void LynxTemplateRenderer::OnEnterBackground() {
  if (shell_->IsDestroyed()) {
    return;
  }
  shell_->OnEnterBackground();
}

void LynxTemplateRenderer::UpdateScreenMetrics(float width, float height,
                                               float display_density) {
  if (shell_->IsDestroyed()) {
    return;
  }
  display_density_ = display_density;
  // The width/height is in pixels, convert to logical size if needed.
  if (ui_delegate_->UsesLogicalPixels()) {
    width = width / display_density_;
    height = height / display_density_;
  }
  float pixel_ratio =
      ui_delegate_->UsesLogicalPixels() ? display_density_ : 1.0f;
  ui_delegate_->OnUpdateScreenMetrics(width, height, pixel_ratio);
  shell_->UpdateScreenMetrics(width, height, pixel_ratio);
}

void LynxTemplateRenderer::MergeGlobalProps(lepus::Value global_props) {
  if (!global_props.IsTable()) {
    return;
  }
  if (global_props_.IsTable()) {
    tasm::ForEachLepusValue(
        global_props_,
        [&global_props](const lepus::Value& key, const lepus::Value& value) {
          if (!global_props.Table()->Contains(key.String())) {
            global_props.SetProperty(key.String(), value);
          }
        });
  }
  global_props_ = std::move(global_props);
}

void LynxTemplateRenderer::UpdateGlobalProps(lepus::Value value) {
  if (!value.IsTable()) {
    return;
  }
  MergeGlobalProps(std::move(value));
  shell_->UpdateGlobalProps(global_props_);
}

void LynxTemplateRenderer::UpdateMetaData(
    const std::shared_ptr<tasm::TemplateData>& data,
    lepus::Value global_props) {
  MergeGlobalProps(std::move(global_props));
  shell_->UpdateMetaData(data, global_props_);
}

void LynxTemplateRenderer::LoadTemplate(
    const std::string& url, std::vector<uint8_t> source,
    const std::shared_ptr<lynx::tasm::PipelineOptions>& pipeline_options,
    const std::shared_ptr<tasm::TemplateData>& template_data,
    bool enable_recycle_template_bundle) {
  if (inspector_owner_ != nullptr) {
    inspector_owner_->OnLoadTemplate(url, source, template_data);
    inspector_owner_->OnLoaded(url);
  }
  pipeline_options->enable_pre_painting = false;
  pipeline_options->enable_recycle_template_bundle =
      enable_recycle_template_bundle;
  shell_->LoadTemplate(url, source, pipeline_options, template_data);
}

void LynxTemplateRenderer::ReloadTemplate(
    const std::shared_ptr<tasm::TemplateData>& data,
    const std::shared_ptr<lynx::tasm::PipelineOptions>& pipeline_options,
    lepus::Value global_props) {
  auto lynx_context =
      static_cast<tasm::harmony::UIDelegateHarmony*>(ui_delegate_)
          ->GetLynxContext();
  if (lynx_context && lynx_context->EnableExposureWhenReload()) {
    lynx_context->StopExposure(lepus::Value());
    lynx_context->ResumeExposure();
  }
  global_props_ = std::move(global_props);
  shell_->ReloadTemplate(data, pipeline_options, global_props_);
}

void LynxTemplateRenderer::LoadTemplateBundle(
    const std::string& url, const lynx::tasm::LynxTemplateBundle& bundle,
    const std::shared_ptr<lynx::tasm::PipelineOptions>& pipeline_options,
    const std::shared_ptr<lynx::tasm::TemplateData>& template_data,
    bool enable_dump_element_tree) {
  pipeline_options->enable_pre_painting = false;
  pipeline_options->enable_dump_element_tree = enable_dump_element_tree;
  shell_->LoadTemplateBundle(url, bundle, pipeline_options, template_data);
}

std::shared_ptr<tasm::PipelineOptions>
LynxTemplateRenderer::ProcessLoadTemplateTimingOption(
    napi_env env, napi_value arg, std::string pipeline_origin) {
  std::shared_ptr<tasm::PipelineOptions> pipeline_options =
      std::make_shared<tasm::PipelineOptions>();
  pipeline_options->pipeline_origin = std::move(pipeline_origin);
  pipeline_options->need_timestamps = true;
  napi_valuetype type;
  napi_typeof(env, arg, &type);
  if (type != napi_object) {
    shell_->OnPipelineStart(pipeline_options->pipeline_id,
                            pipeline_options->pipeline_origin,
                            pipeline_options->pipeline_start_timestamp);
    return pipeline_options;
  }
  // Process TimingOption, generate PipelineOptions
  lepus::Value timing_option;
  /**
   * @see: performance/timing/TimingOption.ets
   * TimingOption: Record<string, string | <string, double>>
   * {
   * "pipelineOrigin": "loadBundle",
   * "timingStampMap": {
   *   "loadBundleStart": 1688308322318,
   *   "pipelineStart": 1688308322318,
   *   "verifyTasmStart": 1688308322318,
   *   "verifyTasmEnd": 1688308322318,
   *   "ffiStart": 1688308322318,
   *    ....
   *  },
   * }
   */
  timing_option = base::NapiConvertHelper::ConvertToLepusValue(env, arg);

  // 1. update `pipeline_origin` if exists.
  auto timing_pipeline_origin = timing_option.GetProperty(
      BASE_STATIC_STRING(tasm::timing::kPipelineOrigin));
  if (timing_pipeline_origin.IsString()) {
    pipeline_options->pipeline_origin = timing_pipeline_origin.StdString();
  }
  // 2. update `pipeline_start_timestamp` if exists.
  auto timestamp_map = timing_option.GetProperty(
      BASE_STATIC_STRING(tasm::timing::kTimestampMap));
  if (!timestamp_map.IsEmpty()) {
    auto timing_pipeline_origin = timestamp_map.GetProperty(
        BASE_STATIC_STRING(tasm::timing::kPipelineStart));
    if (timing_pipeline_origin.IsNumber()) {
      pipeline_options->pipeline_start_timestamp =
          static_cast<uint64_t>(timing_pipeline_origin.Number());
    }
  }
  // 3. start pipeline
  shell_->OnPipelineStart(pipeline_options->pipeline_id,
                          pipeline_options->pipeline_origin,
                          pipeline_options->pipeline_start_timestamp);
  // 4. set all timing
  tasm::ForEachLepusValue(timestamp_map, [this, &pipeline_options](
                                             const lepus::Value& timingKey,
                                             const lepus::Value& timingStamp) {
    this->shell_->SetTiming(static_cast<uint64_t>(timingStamp.Number()),
                            timingKey.StdString(),
                            pipeline_options->pipeline_id);
  });
  return pipeline_options;
}

void LynxTemplateRenderer::ResetTimingBeforeReload() const {
  shell_->ResetTimingBeforeReload();
}

lepus::Value LynxTemplateRenderer::GetAllTimingInfo() const {
  return shell_->GetAllTimingInfo();
}

int32_t LynxTemplateRenderer::GetInstanceId() const {
  return shell_->GetInstanceId();
}

void LynxTemplateRenderer::UpdateFontScale(float font_scale) {
  shell_->UpdateFontScale(font_scale);
}

void LynxTemplateRenderer::SetEnableBytecode(bool enable,
                                             std::string source_url) {
  shell_->SetEnableBytecode(enable, std::move(source_url));
}

lepus::Value LynxTemplateRenderer::GetPageDataByKey(
    std::vector<std::string> keys) {
  return shell_->GetPageDataByKey(std::move(keys));
}

void LynxTemplateRenderer::OnReloadTemplate(
    const std::string& url, const std::vector<uint8_t>& source,
    const std::shared_ptr<tasm::TemplateData>& data) {
  base::NapiHandleScope scope(env_);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "onReloadTemplate", 0, nullptr);
}

void LynxTemplateRenderer::OnLoaded(const std::string& url) {
  base::NapiHandleScope scope(env_);
  napi_value param[1];
  napi_create_string_utf8(env_, url.c_str(), url.length(), &param[0]);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_, "onLoaded",
                                      1, param);
}

void LynxTemplateRenderer::OnRuntimeReady() {
  base::NapiHandleScope scope(env_);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "onRuntimeReady", 0, nullptr);
}

void LynxTemplateRenderer::OnDataUpdated() {
  base::NapiHandleScope scope(env_);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "onDataUpdated", 0, nullptr);
}

void LynxTemplateRenderer::OnPageChanged(bool is_first_screen) {
  base::NapiHandleScope scope(env_);
  napi_value param[1];
  napi_get_boolean(env_, is_first_screen, &param[0]);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "onPageChanged", 1, param);
}
void LynxTemplateRenderer::OnFirstLoadPerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  base::NapiHandleScope scope(env_);
  napi_value param[2];
  param[0] = base::NapiUtil::CreateMap(env_, perf);
  param[1] = base::NapiUtil::CreateMap(env_, perf_timing);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "onFirstLoadPerfReady", 2, param);
}

void LynxTemplateRenderer::OnUpdatePerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  base::NapiHandleScope scope(env_);
  napi_value param[2];
  param[0] = base::NapiUtil::CreateMap(env_, perf);
  param[1] = base::NapiUtil::CreateMap(env_, perf_timing);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "onUpdatePerfReady", 2, param);
}

void LynxTemplateRenderer::OnErrorOccurred(
    int level, int32_t error_code, const std::string& message,
    const std::string& fix_suggestion,
    const std::unordered_map<std::string, std::string>& custom_info,
    bool is_logbox_only) {
  base::NapiHandleScope scope(env_);
  std::string level_str = base::LynxError::GetLevelString(level);
  napi_value param[6];
  napi_create_string_utf8(env_, level_str.c_str(), level_str.length(),
                          &param[0]);
  napi_create_int32(env_, error_code, &param[1]);
  napi_create_string_utf8(env_, message.c_str(), message.length(), &param[2]);
  napi_create_string_utf8(env_, fix_suggestion.c_str(), fix_suggestion.length(),
                          &param[3]);
  param[4] = base::NapiUtil::CreateMap(env_, custom_info);
  napi_get_boolean(env_, is_logbox_only, &param[5]);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "onErrorOccurred", 6, param);
}

void LynxTemplateRenderer::OnThemeUpdatedByJs(
    const std::unordered_map<std::string, std::string>& theme) {
  base::NapiHandleScope scope(env_);
  napi_value param[1];
  param[0] = base::NapiUtil::CreateMap(env_, theme);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "onThemeUpdatedByJs", 1, param);
}

void LynxTemplateRenderer::OnTemplateBundleReady(
    const tasm::LynxTemplateBundle& bundle) {
  base::NapiHandleScope scope(env_);
  napi_value param[2] = {nullptr, nullptr};
  auto status = base::NapiUtil::InvokeJsMethod(env_, template_renderer_ref_,
                                               "createTemplateBundle", 0,
                                               nullptr, &param[0]);
  if (status != napi_ok || param[0] == nullptr) {
    LOGE("create template bundle failed");
    return;
  }

  LynxTemplateBundleHarmony* bundle_harmony = nullptr;
  status =
      napi_unwrap(env_, param[0], reinterpret_cast<void**>(&bundle_harmony));
  if (status == napi_ok && bundle_harmony != nullptr) {
    bundle_harmony->SetBundle(bundle);
    napi_create_string_utf8(env_, "", 0, &param[1]);
  } else {
    LOGE("unwrap bundle failed");
    napi_create_string_utf8(env_, "native TemplateBundle doesn't exist",
                            NAPI_AUTO_LENGTH, &param[1]);
  }

  base::NapiUtil::InvokeJsMethod(env_, template_renderer_ref_,
                                 "onTemplateBundleReady", 2, param, nullptr);
}

#define DECLARE_NAPI_METHOD(name, func) \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

#define NAPI_METHOD_COUNT(properties) sizeof(properties) / sizeof(properties[0])

napi_value LynxTemplateRenderer::Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_METHOD("nativeAttach", NativeAttach),
      DECLARE_NAPI_METHOD("nativeDetach", NativeDetach),
      DECLARE_NAPI_METHOD("nativeReset", NativeReset),
      DECLARE_NAPI_METHOD("updateGlobalProps", UpdateGlobalProps),
      DECLARE_NAPI_METHOD("updateMetaData", UpdateMetaData),
      DECLARE_NAPI_METHOD("loadTemplate", LoadTemplate),
      DECLARE_NAPI_METHOD("reloadTemplate", ReloadTemplate),
      DECLARE_NAPI_METHOD("loadTemplateBundle", LoadTemplateBundle),
      DECLARE_NAPI_METHOD("updateViewport", UpdateViewport),
      DECLARE_NAPI_METHOD("updateScreenMetrics", UpdateScreenMetrics),
      DECLARE_NAPI_METHOD("triggerEventBus", TriggerEventBus),
      DECLARE_NAPI_METHOD("callJSFunction", CallJSFunction),
      DECLARE_NAPI_METHOD("callJSApiCallbackWithValue",
                          CallJSApiCallbackWithValue),
      DECLARE_NAPI_METHOD("callJSIntersectionObserver",
                          CallJSIntersectionObserver),
      DECLARE_NAPI_METHOD("evaluateScript", EvaluateScript),
      DECLARE_NAPI_METHOD("rejectDynamicComponentLoad",
                          RejectDynamicComponentLoad),
      DECLARE_NAPI_METHOD("sendTouchEvent", SendTouchEvent),
      DECLARE_NAPI_METHOD("sendCustomEvent", SendCustomEvent),
      DECLARE_NAPI_METHOD("scrollByListContainer", ScrollByListContainer),
      DECLARE_NAPI_METHOD("scrollToPosition", ScrollToPosition),
      DECLARE_NAPI_METHOD("scrollStopped", ScrollStopped),
      DECLARE_NAPI_METHOD("getAllTimingInfo", GetAllTimingInfo),
      DECLARE_NAPI_METHOD("getInstanceId", GetInstanceId),
      DECLARE_NAPI_METHOD("updateFontScale", UpdateFontScale),
      DECLARE_NAPI_METHOD("nativeSetEnableBytecode", NativeSetEnableBytecode),
      DECLARE_NAPI_METHOD("getPageDataByKey", GetPageDataByKey),
      DECLARE_NAPI_METHOD("setupExtensionDelegate", SetupExtensionDelegate),
      DECLARE_NAPI_METHOD("onEnterForeground", OnEnterForeground),
      DECLARE_NAPI_METHOD("onEnterBackground", OnEnterBackground),
      DECLARE_NAPI_METHOD("nativeGetAllJsSource", GetAllJsSource),
      DECLARE_NAPI_METHOD("invokeLepusCallback", InvokeLepusCallback),
  };

  napi_value cons;
  std::string export_class = "LynxTemplateRenderer";
  napi_define_class(env, export_class.c_str(), NAPI_AUTO_LENGTH, New, nullptr,
                    NAPI_METHOD_COUNT(properties), properties, &cons);

  napi_set_named_property(env, exports, export_class.c_str(), cons);

  NAPI_CREATE_FUNCTION(env, exports, "initGlobalEnv", InitGlobalEnv);
  NAPI_CREATE_FUNCTION(env, exports, "getBaseTraceBackend",
                       GetBaseTraceBackend);
  NAPI_CREATE_FUNCTION(env, exports, "setTracingDirPath", SetTracingDirPath);
  NAPI_CREATE_FUNCTION(env, exports, "traceEventBegin", TraceEventBegin);
  NAPI_CREATE_FUNCTION(env, exports, "traceEventEnd", TraceEventEnd);
  NAPI_CREATE_FUNCTION(env, exports, "traceInstant", TraceInstant);
  NAPI_CREATE_FUNCTION(env, exports, "parserTestBenchRecordData",
                       ParserTestBenchRecordData);
  NAPI_CREATE_FUNCTION(env, exports, "setCacheDirPath", SetCacheDirPath);

  return exports;
}

napi_value LynxTemplateRenderer::GetBaseTraceBackend(napi_env env,
                                                     napi_callback_info info) {
  napi_value result;
  napi_create_int64(env, reinterpret_cast<int64_t>(lynx::base::BaseTraceEvent),
                    &result);
  return result;
}

napi_value LynxTemplateRenderer::GetAllJsSource(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_value args[0];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "GetAllJsSource failed")) {
    return nullptr;
  }
  auto js_source = obj->shell_->GetAllJsSource();
  return base::NapiUtil::CreateMap(env, js_source);
}

napi_value LynxTemplateRenderer::InvokeLepusCallback(napi_env env,
                                                     napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value args[3];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "InvokeLepusCallback failed")) {
    return nullptr;
  }

  int id = base::NapiUtil::ConvertToDouble(env, args[0]);
  auto entry = base::NapiUtil::ConvertToString(env, args[1]);
  auto lepus_value = base::NapiConvertHelper::ConvertToLepusValue(env, args[2]);
  obj->shell_->GetEngineActor()->Act([id, entry, lepus_value](auto& engine) {
    return engine->InvokeLepusCallback(id, entry, lepus_value);
  });
  return nullptr;
}

napi_value LynxTemplateRenderer::InitGlobalEnv(napi_env env,
                                               napi_callback_info info) {
  static std::once_flag instance_once_flag;
  std::call_once(instance_once_flag, [&env, &info]() {
    uv_loop_t* loop;
    napi_get_uv_event_loop(env, &loop);
    lynx::base::UIThread::Init(loop);

    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    harmony::LynxResourceLoaderHarmony::resource_manager =
        OH_ResourceManager_InitNativeResourceManager(env, args[0]);
  });

  // try to load system font
  if (!tasm::LynxEnv::GetInstance().EnableGlobalFontCollection()) {
    tasm::harmony::SystemFontManager::GetInstance().GetSystemFont(
        env, [](std::string font_family, std::string font_path) {
          LOGI("GetSystemFont when InitGlobalEnv!")
        });
  }
  return nullptr;
}

napi_value LynxTemplateRenderer::NativeAttach(napi_env env,
                                              napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  double screen_density;
  napi_get_value_double(env, args[0], &screen_density);

  LynxTemplateRenderer* renderer =
      new LynxTemplateRenderer(env, js_this, screen_density);

  static auto noop_finalizer = [](napi_env env, void* data, void* hint) {
    // An empty implementation of the napi_finalize callback function is
    // provided here. This is because on the HarmonyOS platform, passing
    // nullptr directly as napi_finalize will result in a failure. The
    // native object will not be cleaned up within the napi_finalize
    // function; instead, it will be actively cleaned up through
    // NativeDetach method.
    LOGI("napi_finalize callback.");
  };

  napi_status status =
      napi_wrap(env, js_this, renderer, noop_finalizer, nullptr, nullptr);

  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "NativeAttach failed due to napi_wrap failed!");
  return nullptr;
}

napi_value LynxTemplateRenderer::NativeReset(napi_env env,
                                             napi_callback_info info) {
  napi_value js_this;
  size_t argc = 17;
  napi_value args[17] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  // UIDelegate
  uint64_t delegate_num = base::NapiUtil::ConvertToPtr(env, args[0]);
  auto* delegate_ptr = reinterpret_cast<tasm::UIDelegate*>(delegate_num);

  // providers
  auto resource_loader =
      std::make_shared<LynxResourceLoaderHarmony>(env, args[1]);

  // size
  double screen_width;
  napi_get_value_double(env, args[2], &screen_width);
  double screen_height;
  napi_get_value_double(env, args[3], &screen_height);

  bool is_host_renderer;
  napi_get_value_bool(env, args[4], &is_host_renderer);

  // PerformanceController
  tasm::performance::PerformanceControllerHarmonyJSWrapper*
      js_perf_controller_wrapper;
  napi_unwrap(env, args[5],
              reinterpret_cast<void**>(&js_perf_controller_wrapper));

  int thread_mode = 0;
  napi_get_value_int32(env, args[6], &thread_mode);

  // runtime options

  // js group
  std::string group_id = base::NapiUtil::ConvertToString(env, args[7]);
  bool use_quickjs;
  napi_get_value_bool(env, args[8], &use_quickjs);
  bool enable_js_group_thread;
  napi_get_value_bool(env, args[9], &enable_js_group_thread);
  std::vector<std::string> preload_js_paths;
  base::NapiUtil::ConvertToArrayString(env, args[10], preload_js_paths);

  // bytecode
  bool enable_bytecode = false;
  napi_get_value_bool(env, args[11], &enable_bytecode);
  std::string bytecode_source_url =
      base::NapiUtil::ConvertToString(env, args[12]);

  bool enable_js = true;
  napi_get_value_bool(env, args[13], &enable_js);

  // module
  static constexpr uint32_t kArgsSize = 4;
  napi_value module_args[kArgsSize];
  base::NapiUtil::ConvertToArray(env, args[14], module_args, kArgsSize);
  napi_value sendable_module_args[kArgsSize];
  base::NapiUtil::ConvertToArray(env, args[15], sendable_module_args,
                                 kArgsSize);
  auto module_factory = std::make_unique<ModuleFactoryHarmony>(
      env, module_args, sendable_module_args);

  // LynxRuntimeWrapper
  LynxRuntimeWrapper* runtime_wrapper = nullptr;
  napi_unwrap(env, args[16], reinterpret_cast<void**>(&runtime_wrapper));

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj,
                             "LynxTemplateRenderer NativeReset failed")) {
    return nullptr;
  }
  obj->SetUpLynxShell(
      env, delegate_ptr, resource_loader, static_cast<float>(screen_width),
      static_cast<float>(screen_height), is_host_renderer,
      js_perf_controller_wrapper, thread_mode, std::move(group_id), use_quickjs,
      enable_js_group_thread, std::move(preload_js_paths), enable_bytecode,
      std::move(bytecode_source_url), enable_js, std::move(module_factory),
      runtime_wrapper);
  return nullptr;
}

napi_value LynxTemplateRenderer::SetTracingDirPath(napi_env env,
                                                   napi_callback_info info) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string tracing_dir_path = base::NapiUtil::ConvertToString(env, args[0]);
  lynx::trace::TraceControllerDelegateHarmony::SetTraceDirPath(
      tracing_dir_path);
  tasm::LynxEnv::GetInstance().SetStorageDirectory(tracing_dir_path);
#endif
  return nullptr;
}

napi_value LynxTemplateRenderer::SetCacheDirPath(napi_env env,
                                                 napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string cache_dir = base::NapiUtil::ConvertToString(env, args[0]);
  lynx::piper::cache::JsCacheManagerHarmony::SetCacheDir(cache_dir);
  return nullptr;
}

napi_value LynxTemplateRenderer::TraceEventBegin(napi_env env,
                                                 napi_callback_info info) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  static constexpr int STATIC_ARG_SIZE = 3;
  size_t argc = STATIC_ARG_SIZE;
  napi_value args[STATIC_ARG_SIZE] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string trace_category = base::NapiUtil::ConvertToString(env, args[0]);
  std::string name = base::NapiUtil::ConvertToString(env, args[1]);
  std::unordered_map<std::string, std::string> props{};
  if (argc >= STATIC_ARG_SIZE) {
    base::NapiUtil::ConvertToMap(env, args[2], props);
  }
  TRACE_EVENT_BEGIN(
      trace_category.c_str(), name, [&props](lynx::perfetto::EventContext ctx) {
        for (const auto& pair : props) {
          ctx.event()->add_debug_annotations(pair.first, pair.second);
        }
      });
#endif
  return nullptr;
}

napi_value LynxTemplateRenderer::TraceEventEnd(napi_env env,
                                               napi_callback_info info) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string trace_category = base::NapiUtil::ConvertToString(env, args[0]);
  std::string name = base::NapiUtil::ConvertToString(env, args[1]);
  TRACE_EVENT_END(trace_category.c_str(), [&name](perfetto::EventContext ctx) {
    ctx.event()->set_name(name);
  });
#endif
  return nullptr;
}

napi_value LynxTemplateRenderer::TraceInstant(napi_env env,
                                              napi_callback_info info) {
#if ENABLE_TRACE_PERFETTO
  static constexpr int STATIC_ARG_SIZE = 3;
  size_t argc = STATIC_ARG_SIZE;
  napi_value args[STATIC_ARG_SIZE] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string trace_category = base::NapiUtil::ConvertToString(env, args[0]);
  std::string name = base::NapiUtil::ConvertToString(env, args[1]);
  std::unordered_map<std::string, std::string> props{};
  if (argc >= STATIC_ARG_SIZE) {
    base::NapiUtil::ConvertToMap(env, args[2], props);
  }
  TRACE_EVENT_INSTANT(trace_category.c_str(), nullptr,
                      [&name, &props](perfetto::EventContext ctx) {
                        ctx.event()->set_name(name);
                        for (const auto& pair : props) {
                          ctx.event()->add_debug_annotations(pair.first,
                                                             pair.second);
                        }
                      });
#endif
  return nullptr;
}

napi_value LynxTemplateRenderer::ParserTestBenchRecordData(
    napi_env env, napi_callback_info info) {
  napi_value args[1] = {nullptr};
  size_t argc = 1;
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string source = base::NapiUtil::ConvertToString(env, args[0]);
  napi_value value;
  std::string result;
#if ENABLE_TESTBENCH_REPLAY
  result =
      tasm::replay::TestBenchUtilsEmbedder::ParserTestBenchRecordData(source);
#endif
  napi_create_string_utf8(env, result.c_str(), result.size(), &value);
  return value;
}

napi_value LynxTemplateRenderer::New(napi_env env, napi_callback_info info) {
  napi_value js_this;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  return js_this;
}

napi_value LynxTemplateRenderer::UpdateViewport(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  auto width = base::NapiUtil::ConvertToFloat(env, args[0]);
  auto width_mode = base::NapiUtil::ConvertToInt32(env, args[1]);
  auto height = base::NapiUtil::ConvertToFloat(env, args[2]);
  auto height_mode = base::NapiUtil::ConvertToInt32(env, args[3]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "NativeUpdateViewport failed")) {
    return nullptr;
  }
  obj->UpdateViewport(width, width_mode, height, height_mode);
  return nullptr;
}

napi_value LynxTemplateRenderer::UpdateScreenMetrics(napi_env env,
                                                     napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  auto width = base::NapiUtil::ConvertToFloat(env, args[0]);
  auto height = base::NapiUtil::ConvertToFloat(env, args[1]);
  auto scale = base::NapiUtil::ConvertToFloat(env, args[2]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "NativeUpdateScreenMetrics failed")) {
    return nullptr;
  }
  obj->UpdateScreenMetrics(width, height, scale);
  return nullptr;
}

napi_value LynxTemplateRenderer::NativeDetach(napi_env env,
                                              napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* renderer = nullptr;
  napi_status status =
      napi_remove_wrap(env, js_this, reinterpret_cast<void**>(&renderer));
  if (!CheckNapiUnwrapObject(status, renderer, "NativeDetach failed")) {
    return nullptr;
  }
  delete renderer;
  return nullptr;
}

napi_value LynxTemplateRenderer::UpdateGlobalProps(napi_env env,
                                                   napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "UpdateGlobalProps failed")) {
    return nullptr;
  }

  lepus_value global_props =
      base::NapiConvertHelper::JSONToLepusValue(env, args[0]);
  obj->UpdateGlobalProps(std::move(global_props));

  return nullptr;
}

napi_value LynxTemplateRenderer::UpdateMetaData(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "UpdateMetaData failed")) {
    return nullptr;
  }

  auto template_data = tasm::TemplateDataHarmony::GenerateTemplateData(
      env, args[1], args[2], args[0]);

  lepus_value global_props =
      base::NapiConvertHelper::JSONToLepusValue(env, args[3]);
  obj->UpdateMetaData(template_data, std::move(global_props));
  return nullptr;
}

napi_value LynxTemplateRenderer::LoadTemplate(napi_env env,
                                              napi_callback_info info) {
  napi_value js_this;
  size_t argc = 7;
  napi_value args[7] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "LoadTemplate failed")) {
    return nullptr;
  }

  std::string url = base::NapiUtil::ConvertToString(env, args[0]);
  std::vector<uint8_t> source;
  base::NapiUtil::ConvertToArrayBuffer(env, args[1], source);

  auto template_data = tasm::TemplateDataHarmony::GenerateTemplateData(
      env, args[3], args[4], args[2]);

  napi_valuetype type;
  napi_typeof(env, args[5], &type);
  bool enable_recycle_template_bundle = false;
  if (type == napi_boolean) {
    enable_recycle_template_bundle =
        base::NapiUtil::ConvertToBoolean(env, args[5]);
  }

  auto pipeline_options = obj->ProcessLoadTemplateTimingOption(
      env, args[6], tasm::timing::kLoadBundle);
  obj->LoadTemplate(url, std::move(source), pipeline_options, template_data,
                    enable_recycle_template_bundle);

  return nullptr;
}

napi_value LynxTemplateRenderer::ReloadTemplate(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 5;
  napi_value args[5] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "ReloadTemplate failed")) {
    return nullptr;
  }

  auto template_data = tasm::TemplateDataHarmony::GenerateTemplateData(
      env, args[1], args[2], args[0]);

  lepus_value global_props =
      base::NapiConvertHelper::JSONToLepusValue(env, args[3]);

  obj->ResetTimingBeforeReload();
  auto pipeline_options = obj->ProcessLoadTemplateTimingOption(
      env, args[4], tasm::timing::kReloadBundleFromNative);
  pipeline_options->is_reload_template = true;
  obj->ReloadTemplate(template_data, pipeline_options, std::move(global_props));
  return nullptr;
}

napi_value LynxTemplateRenderer::LoadTemplateBundle(napi_env env,
                                                    napi_callback_info info) {
  napi_value js_this;
  size_t argc = 7;
  napi_value args[7] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "LoadTemplateBundle failed")) {
    return nullptr;
  }

  std::string url = base::NapiUtil::ConvertToString(env, args[0]);

  LynxTemplateBundleHarmony* bundle = nullptr;
  status = napi_unwrap(env, args[1], reinterpret_cast<void**>(&bundle));
  if (bundle == nullptr) {
    LOGE("get template bundle nullptr");
    return nullptr;
  }

  auto template_data = tasm::TemplateDataHarmony::GenerateTemplateData(
      env, args[3], args[4], args[2]);

  napi_valuetype type;
  napi_typeof(env, args[5], &type);
  bool enable_dump_element_tree = false;
  if (type == napi_boolean) {
    enable_dump_element_tree = base::NapiUtil::ConvertToBoolean(env, args[5]);
  }

  auto pipeline_options = obj->ProcessLoadTemplateTimingOption(
      env, args[6], tasm::timing::kLoadBundle);
  obj->LoadTemplateBundle(std::move(url), bundle->GetBundle(), pipeline_options,
                          template_data, enable_dump_element_tree);
  return nullptr;
}

napi_value LynxTemplateRenderer::GetAllTimingInfo(napi_env env,
                                                  napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_value args[0];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "GetAllTimingInfo failed")) {
    return nullptr;
  }
  const auto result =
      base::NapiConvertHelper::CreateNapiValue(env, obj->GetAllTimingInfo());
  return result;
}

napi_value LynxTemplateRenderer::GetInstanceId(napi_env env,
                                               napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_value args[0];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  napi_value result;
  if (!CheckNapiUnwrapObject(status, obj, "GetInstanceId failed")) {
    napi_create_int32(env, tasm::report::kUnknownInstanceId, &result);
  } else {
    napi_create_int32(env, obj->GetInstanceId(), &result);
  }
  return result;
}

napi_value LynxTemplateRenderer::CallJSFunction(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  std::string module_id = base::NapiUtil::ConvertToShortString(env, args[0]);
  std::string method = base::NapiUtil::ConvertToShortString(env, args[1]);

  auto lepus_value = base::NapiConvertHelper::ConvertToLepusValue(env, args[2]);
  auto params = std::make_unique<pub::ValueImplLepus>(std::move(lepus_value));

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "CallJSFunction failed")) {
    return nullptr;
  }
  if (!obj->runtime_proxy_) {
    return nullptr;
  }
  obj->runtime_proxy_->CallJSFunction(module_id, method, std::move(params));

  return nullptr;
}

napi_value LynxTemplateRenderer::TriggerEventBus(napi_env env,
                                                 napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  std::string name = base::NapiUtil::ConvertToShortString(env, args[0]);
  auto params = base::NapiConvertHelper::ConvertToLepusValue(env, args[1]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "TriggerEventBus failed")) {
    return nullptr;
  }
  obj->shell_->TriggerEventBus(name, params);
  return nullptr;
}

napi_value LynxTemplateRenderer::CallJSApiCallbackWithValue(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  int32_t callback_id = base::NapiUtil::ConvertToInt32(env, args[0]);
  auto lepus_value = base::NapiConvertHelper::ConvertToLepusValue(env, args[1]);
  auto params = std::make_unique<pub::ValueImplLepus>(std::move(lepus_value));

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj,
                             "CallJSApiCallbackWithValue failed")) {
    return nullptr;
  }
  if (!obj->runtime_proxy_) {
    return nullptr;
  }
  obj->runtime_proxy_->CallJSApiCallbackWithValue(callback_id,
                                                  std::move(params));
  return nullptr;
}

napi_value LynxTemplateRenderer::CallJSIntersectionObserver(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  int32_t observer_id = base::NapiUtil::ConvertToInt32(env, args[0]);
  int32_t callback_id = base::NapiUtil::ConvertToInt32(env, args[1]);
  auto lepus_value = base::NapiConvertHelper::ConvertToLepusValue(env, args[2]);
  auto params = std::make_unique<pub::ValueImplLepus>(std::move(lepus_value));

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj,
                             "CallJSIntersectionObserver failed")) {
    return nullptr;
  }
  if (!obj->runtime_proxy_) {
    return nullptr;
  }
  obj->runtime_proxy_->CallJSIntersectionObserver(observer_id, callback_id,
                                                  std::move(params));

  return nullptr;
}

napi_value LynxTemplateRenderer::EvaluateScript(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  std::string url = base::NapiUtil::ConvertToString(env, args[0]);
  std::string script = base::NapiUtil::ConvertToString(env, args[1]);
  int32_t callback_id = base::NapiUtil::ConvertToInt32(env, args[2]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "EvaluateScript failed")) {
    return nullptr;
  }
  if (!obj->runtime_proxy_) {
    return nullptr;
  }
  obj->runtime_proxy_->EvaluateScript(url, std::move(script), callback_id);

  return nullptr;
}

napi_value LynxTemplateRenderer::RejectDynamicComponentLoad(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  std::string url = base::NapiUtil::ConvertToString(env, args[0]);
  int32_t callback_id = base::NapiUtil::ConvertToInt32(env, args[1]);
  int32_t err_code = base::NapiUtil::ConvertToInt32(env, args[2]);
  std::string err_msg = base::NapiUtil::ConvertToShortString(env, args[3]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj,
                             "RejectDynamicComponentLoad failed")) {
    return nullptr;
  }
  if (!obj->runtime_proxy_) {
    return nullptr;
  }
  obj->runtime_proxy_->RejectDynamicComponentLoad(url, callback_id, err_code,
                                                  err_msg);
  return nullptr;
}

napi_value LynxTemplateRenderer::SendTouchEvent(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 8;
  napi_value args[8] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  std::string name = base::NapiUtil::ConvertToShortString(env, args[0]);
  int32_t tag = base::NapiUtil::ConvertToInt32(env, args[1]);
  float x = base::NapiUtil::ConvertToFloat(env, args[2]);
  float y = base::NapiUtil::ConvertToFloat(env, args[3]);
  float client_x = base::NapiUtil::ConvertToFloat(env, args[4]);
  float client_y = base::NapiUtil::ConvertToFloat(env, args[5]);
  float page_x = base::NapiUtil::ConvertToFloat(env, args[6]);
  float page_y = base::NapiUtil::ConvertToFloat(env, args[7]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "SendTouchEvent failed")) {
    return nullptr;
  }
  obj->shell_->SendTouchEvent(name, tag, x, y, client_x, client_y, page_x,
                              page_y);
  return nullptr;
}

napi_value LynxTemplateRenderer::SendCustomEvent(napi_env env,
                                                 napi_callback_info info) {
  napi_value js_this;
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  std::string name = base::NapiUtil::ConvertToShortString(env, args[0]);
  int32_t tag = base::NapiUtil::ConvertToInt32(env, args[1]);
  lepus_value params =
      base::NapiConvertHelper::ConvertToLepusValue(env, args[2]);
  std::string param_name = base::NapiUtil::ConvertToShortString(env, args[3]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "SendCustomEvent failed")) {
    return nullptr;
  }
  obj->shell_->SendCustomEvent(name, tag, params, param_name);
  return nullptr;
}

napi_value LynxTemplateRenderer::ScrollByListContainer(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 5;
  napi_value args[5] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "ScrollByListContainer failed")) {
    return nullptr;
  }

  int32_t tag = base::NapiUtil::ConvertToInt32(env, args[0]);
  float x = base::NapiUtil::ConvertToFloat(env, args[1]);
  float y = base::NapiUtil::ConvertToFloat(env, args[2]);
  float original_x = base::NapiUtil::ConvertToFloat(env, args[3]);
  float original_y = base::NapiUtil::ConvertToFloat(env, args[4]);
  obj->shell_->ScrollByListContainer(tag, x, y, original_x, original_y);

  return nullptr;
}

napi_value LynxTemplateRenderer::ScrollToPosition(napi_env env,
                                                  napi_callback_info info) {
  napi_value js_this;
  size_t argc = 5;
  napi_value args[5] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "ScrollToPosition failed")) {
    return nullptr;
  }

  int32_t tag = base::NapiUtil::ConvertToInt32(env, args[0]);
  int index = base::NapiUtil::ConvertToInt32(env, args[1]);
  float offset = base::NapiUtil::ConvertToFloat(env, args[2]);
  int32_t align = base::NapiUtil::ConvertToInt32(env, args[3]);
  bool smooth = base::NapiUtil::ConvertToBoolean(env, args[4]);
  obj->shell_->ScrollToPosition(tag, index, offset, align, smooth);

  return nullptr;
}

napi_value LynxTemplateRenderer::ScrollStopped(napi_env env,
                                               napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "ScrollStopped failed")) {
    return nullptr;
  }

  int32_t tag = base::NapiUtil::ConvertToInt32(env, args[0]);
  obj->shell_->ScrollStopped(tag);

  return nullptr;
}

napi_value LynxTemplateRenderer::UpdateFontScale(napi_env env,
                                                 napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  auto scale = base::NapiUtil::ConvertToFloat(env, args[0]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "NativeUpdateFontScale failed")) {
    return nullptr;
  }
  obj->UpdateFontScale(scale);
  return nullptr;
}

napi_value LynxTemplateRenderer::NativeSetEnableBytecode(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  bool enable = false;
  napi_get_value_bool(env, args[0], &enable);
  std::string source_url = base::NapiUtil::ConvertToString(env, args[1]);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "NativeUpdateFontScale failed")) {
    return nullptr;
  }
  obj->SetEnableBytecode(enable, source_url);

  return nullptr;
}

napi_value LynxTemplateRenderer::GetPageDataByKey(napi_env env,
                                                  napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  std::vector<std::string> keys;
  base::NapiUtil::ConvertToArrayString(env, args[0], keys);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "GetPageDataByKey failed")) {
    return nullptr;
  }
  auto result = base::NapiConvertHelper::CreateNapiValue(
      env, obj->GetPageDataByKey(std::move(keys)));

  return result;
}

napi_value LynxTemplateRenderer::SetupExtensionDelegate(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  auto delegate_num = base::NapiUtil::ConvertToPtr(env, args[0]);
  auto* delegate_ptr =
      reinterpret_cast<pub::LynxExtensionDelegate*>(delegate_num);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "SetupExtensionDelegate failed")) {
    return nullptr;
  }

  obj->SetupExtensionDelegate(delegate_ptr);
  return nullptr;
}

napi_value LynxTemplateRenderer::OnEnterForeground(napi_env env,
                                                   napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_value args[0];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "OnEnterForeground failed")) {
    return nullptr;
  }
  obj->OnEnterForeground();
  return nullptr;
}

napi_value LynxTemplateRenderer::OnEnterBackground(napi_env env,
                                                   napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_value args[0];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  LynxTemplateRenderer* obj = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!CheckNapiUnwrapObject(status, obj, "OnEnterBackground failed")) {
    return nullptr;
  }
  obj->OnEnterBackground();
  return nullptr;
}

void LynxTemplateRenderer::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  // Main thread
  ui_delegate_->OnPageConfigDecoded(config);
}

lepus::Value LynxTemplateRenderer::TriggerLepusMethod(
    const std::string& method_name, const lepus::Value& args) {
  base::NapiHandleScope scope(env_);
  napi_value param[2];
  napi_create_string_utf8(env_, method_name.c_str(), method_name.length(),
                          &param[0]);
  napi_value result;
  param[1] = base::NapiConvertHelper::CreateNapiValue(env_, args);
  base::NapiUtil::InvokeJsMethod(env_, template_renderer_ref_,
                                 "triggerLepusMethod", 2, param, &result);
  return base::NapiConvertHelper::ConvertToLepusValue(env_, result);
}

void LynxTemplateRenderer::TriggerLepusMethodAsync(
    const std::string& method_name, const lepus::Value& args) {
  base::NapiHandleScope scope(env_);
  napi_value param[2];
  napi_create_string_utf8(env_, method_name.c_str(), method_name.length(),
                          &param[0]);
  param[1] = base::NapiConvertHelper::CreateNapiValue(env_, args);
  base::NapiUtil::AsyncInvokeJsMethod(env_, template_renderer_ref_,
                                      "triggerLepusMethodAsync", 2, param);
}

void LynxTemplateRenderer::ReloadTemplate(
    const std::string& url, const std::vector<uint8_t>& source,
    const std::shared_ptr<tasm::TemplateData>& data) {
  OnReloadTemplate(url, source, data);
}

std::vector<uint8_t> LynxTemplateRenderer::LoadJSSource(
    const std::string& url) {
  if (!resource_loader_) {
    return std::vector<uint8_t>();
  }
  std::promise<std::vector<uint8_t>> promise;
  std::future<std::vector<uint8_t>> future = promise.get_future();
  auto request = pub::LynxResourceRequest{
      .url = url, .type = pub::LynxResourceType::kAssets};
  resource_loader_->LoadResource(
      request, [promise = std::move(promise)](
                   pub::LynxResourceResponse& response) mutable {
        promise.set_value(std::move(response.data));
      });
  return future.get();
}

void LynxTemplateRenderer::LoadTemplateFromURL(
    const std::string& url,
    const std::shared_ptr<tasm::TemplateData> init_data) {
  // TODO(zhangkaijie.9): replace nullptr to std::shared<tasm::PipelineOptions>
  std::shared_ptr<lynx::tasm::PipelineOptions> pipeline_options = nullptr;
  static const char* FILE_SCHEME = "file://";
  static const char* ASSETS_SCHEME = "assets://";
  if (memcmp(url.c_str(), FILE_SCHEME, strlen(FILE_SCHEME)) == 0 ||
      memcmp(url.c_str(), ASSETS_SCHEME, strlen(ASSETS_SCHEME)) == 0) {
    LoadTemplate(url, LoadJSSource(url), pipeline_options, init_data, false);
    return;
  }

  if (!resource_loader_) {
    return;
  }

  pub::LynxResourceRequest req{.url = url,
                               .type = pub::LynxResourceType::kTemplate};
  resource_loader_->LoadResource(
      req, [weak_flag = weak_flag_->weak_from_this(), url, init_data,
            &pipeline_options](pub::LynxResourceResponse& response) {
        auto flag = weak_flag.lock();
        if (!flag) {
          return;
        }
        auto data_size = response.data.size();
        LOGI("LoadTemplateFromURL data_size: " << data_size);
        flag->renderer->LoadTemplate(url, std::move(response.data),
                                     pipeline_options, init_data, false);
      });
}

double LynxTemplateRenderer::GetScreenScaleFactor() { return display_density_; }

void LynxTemplateRenderer::TakeSnapshot(
    size_t max_width, size_t max_height, int quality, float screen_scale_factor,
    const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
    tasm::TakeSnapshotCompletedCallback callback) {
  ui_delegate_->TakeSnapshot(max_width, max_height, quality,
                             screen_scale_factor, screenshot_runner, callback);
}

int LynxTemplateRenderer::GetNodeForLocation(int x, int y) {
  return ui_delegate_->GetNodeForLocation(x, y);
}

std::vector<float> LynxTemplateRenderer::GetTransformValue(
    int id, const std::vector<float>& pad_border_margin_layout) {
  return ui_delegate_->GetTransformValue(id, pad_border_margin_layout);
}

void LynxTemplateRenderer::SetupExtensionDelegate(
    pub::LynxExtensionDelegate* delegate) {
  delegate->SetRuntimeActor(shell_->GetRuntimeActor());
}

void LynxTemplateRenderer::SetInspectorOwner(
    devtool::LynxInspectorOwner* owner) {
  inspector_owner_ = owner;
}

}  // namespace harmony
}  // namespace lynx
