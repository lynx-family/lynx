// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/template_entry.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/tasm/i18n/i18n.h"
#include "core/renderer/template_assembler.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/profile/lepusng/lepusng_profiler.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/services/event_report/event_tracker.h"

#if ENABLE_LEPUSNG_WORKLET
#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/bindings/napi/worklet/napi_loader_ui.h"
#endif

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace tasm {

TemplateEntry::TemplateEntry() : VmContextHolder(nullptr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntry::TemplateEntry");
  template_bundle_.css_style_manager_ =
      std::make_shared<CSSStyleSheetManager>(this);
}

TemplateEntry::TemplateEntry(const std::shared_ptr<lepus::Context>& context,
                             const std::string& targetSdkVersion)
    : VmContextHolder(context) {
  template_bundle_.css_style_manager_ =
      std::make_shared<CSSStyleSheetManager>(this);

  vm_context_->SetSdkVersion(targetSdkVersion);
  vm_context_->Initialize();
}

bool TemplateEntry::ConstructContext(TemplateAssembler* assembler,
                                     bool is_lepusng_binary,
                                     const lepus::ContextBundle& context_bundle,
                                     bool use_context_pool,
                                     bool disable_tracing_gc) {
  auto source_type = LepusContextSourceType::kFromRuntime;
  if (((is_lepusng_binary && use_context_pool) ||
       template_bundle().EnableUseContextPool()) &&
      !disable_tracing_gc) {
    // 1. try to take context for local pool
    if (template_bundle().quick_context_pool_) {
      vm_context_ = template_bundle().quick_context_pool_->TakeContextSafely();
    }
    if (vm_context_) {
      source_type = LepusContextSourceType::kFromLocalPool;
    } else {
      // 2. try to take context for global pool
      vm_context_ = LynxGlobalPool::GetInstance()
                        .GetQuickContextPool()
                        .TakeContextSafely();
      source_type =
          vm_context_ ? LepusContextSourceType::kFromGlobalPool : source_type;
    }

    tasm::report::EventTracker::OnEvent(
        [source_type](tasm::report::MoveOnlyEvent& event) {
          event.SetName("quick_context_pre_create");
          event.SetProps(
              "use_global_context_pool",
              source_type == LepusContextSourceType::kFromGlobalPool);
          event.SetProps("use_bundle_context_pool",
                         source_type == LepusContextSourceType::kFromLocalPool);
        });
  }

  // 3. construct a context at runtime
  if (!vm_context_) {
    vm_context_ =
        lepus::Context::CreateContext(is_lepusng_binary, disable_tracing_gc);
  }

  if (!vm_context_) {
    return false;
  }

  vm_context_->SetSdkVersion(assembler->target_sdk_version_);
  vm_context_->Initialize();
#if ENABLE_TRACE_PERFETTO
  if (is_lepusng_binary) {
    std::shared_ptr<lepus::QuickContext> context =
        std::static_pointer_cast<lepus::QuickContext>(vm_context_);
    auto profiler = std::make_shared<profile::LepusNGProfiler>(context);
    context->SetRuntimeProfiler(profiler);
  }
#endif
  RegisterBuiltin(assembler);
  std::string file_name = GenerateLepusJSFileName(name_);
  // the context from local pool has no need to DeSerialize
  return source_type == LepusContextSourceType::kFromLocalPool ||
         vm_context_->DeSerialize(context_bundle, false, nullptr,
                                  file_name.c_str());
}

std::unique_ptr<TemplateEntry>
TemplateEntry::ConstructEntryWithNoTemplateAssembler(
    std::shared_ptr<lepus::Context> context,
    const std::string& targetSdkVersion) {
  return std::make_unique<TemplateEntry>(context, targetSdkVersion);
}

bool TemplateEntry::InitWithTemplateBundle(
    TemplateBinaryReader::PageConfigger* configger,
    LynxTemplateBundle template_bundle) {
  SetTemplateBundle(std::move(template_bundle));
  return InitWithPageConfigger(configger);
}

void TemplateEntry::SetTemplateBundle(LynxTemplateBundle template_bundle) {
  template_bundle_ = std::move(template_bundle);

  // TODO(zhoupeng): CSSStyleSheetManager needs to lock when trying to get a css
  // fragment, so we should not use this manager directly, but only copy the
  // data in it. CSSStyleSheetManager as a runtime manager contains a lot of
  // unnecessary logic. Or it shouldn't be in the template bundle. Optimize it
  // later.
  auto css_manager = std::make_shared<CSSStyleSheetManager>(nullptr);
  css_manager->CopyFrom(*template_bundle_.css_style_manager_);
  template_bundle_.css_style_manager_ = std::move(css_manager);

  is_template_bundle_complete_ = true;
}

std::string TemplateEntry::GenerateLepusJSFileName(const std::string& name) {
  static const char* kLepusFilePrefix = "file://";
  static const char* kLepusFileSuffix = "/lepus.js";
  return kLepusFilePrefix + name + kLepusFileSuffix;
}

bool TemplateEntry::InitWithPageConfigger(
    TemplateBinaryReader::PageConfigger* configger) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntry::InitWithPageConfigger");

  if (is_card_ != template_bundle_.IsCard()) {
    // expected type does not match actual type
    error_msg_ = "Template bundle type mismatch, expect type: " +
                 std::to_string(is_card_) +
                 ", actual type: " + std::to_string(template_bundle_.IsCard());
    return false;
  }

  auto page_config = EnsurePageConfig(configger);
  if (!page_config) {
    constexpr char kPageConfigNullptr[] = "PageConfig is nullptr";
    error_msg_ = kPageConfigNullptr;
    return false;
  }

  // lazy construct lepus context.
  if (!InitLepusContext(static_cast<TemplateAssembler*>(configger),
                        page_config)) {
    return false;
  }

  if (is_card_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "InitCardEnv");
    configger->SetSupportComponentJS(template_bundle_.support_component_js_);
    configger->SetTargetSdkVersion(template_bundle_.target_sdk_version_);
    configger->Themed().ResetWithPageTransMaps(
        template_bundle_.themed_.pageTransMaps);
  }

  SetCircularDataCheck(page_config->GetGlobalCircularDataCheck());

  SetEnableJsBindingApiThrowException(
      page_config->GetEnableJsBindingApiThrowException());

  vm_context_->SetSdkVersion(compile_options().target_sdk_version_);

  ApplyConfigsToLepusContext(page_config);

  if (is_card_) {
    // ApplyConfigsToLepusContext() will set template_debug_url_ to vm_context_,
    // InitInspector() must be called after that.
    vm_context_->InitInspector(lepus_observer_.lock());
  }

  if (page_config->GetEnableBindICU()) {
    SetEnableBindICU(true);
#if ENABLE_LEPUSNG_WORKLET
    Napi::Env env = napi_environment_->proxy()->Env();
    I18n::Bind(reinterpret_cast<intptr_t>(static_cast<napi_env>(env)));
#endif
  }

  SetEnableMicrotaskPromisePolyfill(
      page_config->GetEnableMicrotaskPromisePolyfill());
  return true;
}

bool TemplateEntry::InitLepusContext(
    TemplateAssembler* tasm, const std::shared_ptr<PageConfig>& page_config) {
  if (vm_context_) {
    return true;
  }

  if (!template_bundle().context_bundle_) {
    constexpr char kContextBundleNullptr[] = "Context bundle is nullptr";
    error_msg_ = kContextBundleNullptr;
    return false;
  }

  if (EnableReuseContext()) {
    // reuse lepus context
    const auto& page_context = tasm->getLepusContext(DEFAULT_ENTRY_NAME);
    if (!(template_bundle_.is_lepusng_binary_ &&
          page_context->IsLepusNGContext())) {
      // only supported in lepusNG
      constexpr char kReuseContextFailed[] =
          "reuse lepus context only supported in lepusNG";
      error_msg_ = kReuseContextFailed;
      return false;
    }
    SetVm(page_context);
    std::string file_name = GenerateLepusJSFileName(name_);
    if (!vm_context_->DeSerialize(*template_bundle().context_bundle_, true,
                                  &binary_eval_result_, file_name.c_str())) {
      constexpr char kContextDeSerializeFailed[] = "Context DeSerialize failed";
      error_msg_ = kContextDeSerializeFailed;
      return false;
    }
    return true;
  }

  if (!ConstructContext(tasm, template_bundle().is_lepusng_binary(),
                        *template_bundle().context_bundle_,
                        page_config->GetEnableUseContextPool(),
                        page_config->GetDisableQuickTracingGC())) {
    constexpr char kContextConstructFailed[] = "Context construct failed";
    error_msg_ = kContextConstructFailed;
    return false;
  }

  // for card: the entry name is the app_name_, the context name is
  // DEFAULT_ENTRY_NAME;
  // for lazy bundle: the entry name and context name are the url set
  // in runtime, have nothing to do with app_name_;
  if (is_card_) {
    SetName(template_bundle_.app_name_);
    vm_context_->set_name(DEFAULT_ENTRY_NAME);
  } else {
    vm_context_->set_name(GetName());
  }

  return true;
}

std::shared_ptr<PageConfig> TemplateEntry::EnsurePageConfig(
    TemplateBinaryReader::PageConfigger* configger) const {
  if (is_card_) {
    // since native config is supported now, we need to make a clone to
    // pageConfig in order to avoid native config modifies the page config in
    // bundle.
    configger->SetPageConfig(
        std::make_shared<PageConfig>(*template_bundle_.page_configs_));
  }
  return configger->GetPageConfig();
}

lepus::Value TemplateEntry::ProcessBinaryEvalResult() {
  if (vm_context_ && EnableReuseContext() && !binary_eval_result_.IsNil()) {
    // for lazy bundle 3.0, we need to process the evalResult, handled by
    // fe `globalThis.processEvalResult`
    BASE_STATIC_STRING_DECL(kProcessEvalResult, "processEvalResult");
    auto* context = static_cast<lepus::QuickContext*>(vm_context_.get());
    if (!context->GetGlobalData(kProcessEvalResult).IsEmpty()) {
      return context->Call(kProcessEvalResult, binary_eval_result_,
                           lepus::Value(GetName()));
    }
  }
  return binary_eval_result_;
}

void TemplateEntry::ApplyConfigsToLepusContext(
    const std::shared_ptr<PageConfig>& page_config) {
  vm_context_->ApplyConfig(page_config, compile_options());
  if (page_config->GetLynxAirMode() != CompileOptionAirMode::AIR_MODE_STRICT) {
    AttachNapiEnvironment();
  }
  return;
}

bool TemplateEntry::Execute() {
  if (is_card_ || !EnableReuseContext()) {
    return GetVm()->Execute();
  }
  binary_eval_result_ = ProcessBinaryEvalResult();
  // Binary is already executed while EvalBinary.
  return true;
}

TemplateEntry::~TemplateEntry() {
  DetachNapiEnvironment();
  template_bundle_.css_style_manager_->SetThreadStopFlag(true);
  template_bundle_.lepus_chunk_manager_->SetThreadStopFlag(true);
#if ENABLE_TRACE_PERFETTO
  if (vm_context_ && vm_context_->IsLepusNGContext()) {
    auto context = std::static_pointer_cast<lepus::QuickContext>(vm_context_);
    context->RemoveRuntimeProfiler();
  }
#endif
}

void TemplateEntry::RegisterBuiltin(TemplateAssembler* assembler) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntry::RegisterBuiltin");
  BASE_STATIC_STRING_DECL(kTemplateAssembler, "$kTemplateAssembler");
  vm_context_->SetGlobalData(
      kTemplateAssembler,
      lepus::Value(static_cast<lepus::Context::Delegate*>(assembler)));
  vm_context_->RegisterCtxBuiltin(compile_options().arch_option_);
  return;
}

const ElementTemplateInfo& TemplateEntry::GetElementTemplateInfo(
    const std::string& key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntry::GetElementTemplateInfo");
  auto iter = template_bundle_.element_template_infos_.find(key);
  if (iter == template_bundle_.element_template_infos_.end()) {
    auto info = reader_ ? reader_->DecodeElementTemplateInRender(key)
                        : std::make_shared<ElementTemplateInfo>();
    auto res =
        template_bundle_.element_template_infos_.emplace(key, std::move(info));
    return *(res.first->second);
  }
  return *(iter->second);
}

const std::shared_ptr<ParsedStyles>& TemplateEntry::GetParsedStyles(
    const std::string& key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntry::GetParsedStyles", "key",
              key);
  if (reader_) {
    return reader_->GetParsedStylesInRender(key);
  }
  auto iter = template_bundle_.parsed_styles_map_.find(key);
  if (iter == template_bundle_.parsed_styles_map_.end()) {
    auto res = template_bundle_.parsed_styles_map_.emplace(
        key, std::make_shared<ParsedStyles>());
    return res.first->second;
  }
  return iter->second;
}

const AirCompStylesMap& TemplateEntry::GetComponentParsedStyles(
    const std::string& path) {
  return template_bundle_.air_parsed_styles_map_[path];
}

void TemplateEntry::SetName(const std::string& name) {
  name_ = name;
  if (vm_context_) {
    vm_context_->set_name(name);
  }
}

void TemplateEntry::AddLazyBundleDeclaration(const std::string& name,
                                             const std::string& path) {
  template_bundle_.dynamic_component_declarations_[name] = path;
}

void TemplateEntry::ReInit(TemplateAssembler* assembler) {
  vm_context_->Initialize();
  RegisterBuiltin(assembler);
}

piper::NapiEnvironment* TemplateEntry::napi_environment() {
#if ENABLE_LEPUSNG_WORKLET
  return napi_environment_.get();
#else
  return nullptr;
#endif
}

void TemplateEntry::InvokeLepusBridge(const int32_t callback_id,
                                      const lepus::Value& data) {
#if ENABLE_LEPUSNG_WORKLET
  reinterpret_cast<lynx::worklet::NapiLoaderUI*>(napi_environment_->delegate())
      ->InvokeLepusBridge(callback_id, data);
#endif
}

void TemplateEntry::AttachNapiEnvironment() {
#if ENABLE_LEPUSNG_WORKLET
  if (vm_context_->IsLepusNGContext() && !napi_environment_) {
    lepus::QuickContext* qctx = lepus::QuickContext::Cast(vm_context_.get());
    napi_environment_ = std::make_unique<lynx::piper::NapiEnvironment>(
        std::make_unique<lynx::worklet::NapiLoaderUI>(qctx));
    auto proxy = lynx::piper::NapiRuntimeProxyQuickjs::Create(qctx->context());
    auto napi_proxy = std::unique_ptr<piper::NapiRuntimeProxy>(
        static_cast<piper::NapiRuntimeProxy*>(proxy.release()));
    napi_environment_->SetRuntimeProxy(std::move(napi_proxy));
    napi_environment_->Attach();
  }
#endif
}

void TemplateEntry::DetachNapiEnvironment() {
#if ENABLE_LEPUSNG_WORKLET
  if (vm_context_ && vm_context_->IsLepusNGContext() && napi_environment_) {
    napi_environment_->Detach();
  }
#endif
}

bool TemplateEntry::IsCompatibleWithRootEntry(const TemplateEntry& root,
                                              std::string& msg) {
  const auto& component_compile_option = compile_options();
  const auto& root_compile_options = root.compile_options();
  if (component_compile_option.radon_mode_ !=
      root_compile_options.radon_mode_) {
    msg = "LazyBundle's radon mode is: " +
          std::to_string(component_compile_option.radon_mode_) +
          ", while the root's radon mode is: " +
          std::to_string(root_compile_options.radon_mode_);
    return false;
  }
  if (component_compile_option.front_end_dsl_ !=
      root_compile_options.front_end_dsl_) {
    msg = "LazyBundle's dsl is: " +
          std::to_string(component_compile_option.front_end_dsl_) +
          ", while the root's dsl is: " +
          std::to_string(root_compile_options.front_end_dsl_);
    return false;
  }

  if (component_compile_option.arch_option_ !=
      root_compile_options.arch_option_) {
    msg = "LazyBundle's ArchOption is: " +
          std::to_string(component_compile_option.arch_option_) +
          ", while the root's ArchOption is: " +
          std::to_string(root_compile_options.arch_option_);
    return false;
  }
  if (component_compile_option.enable_css_parser_ !=
      root_compile_options.enable_css_parser_) {
    msg = "LazyBundle's enable_css_parser_ is: " +
          std::to_string(component_compile_option.enable_css_parser_) +
          ", while the root's enable_css_parser_ is: " +
          std::to_string(root_compile_options.enable_css_parser_);
    return false;
  }
  return true;
}

TasmRuntimeBundle TemplateEntry::CreateTasmRuntimeBundle() {
  // In fiber mode, 'page_moulds_' is always empty, and 'encoded_data' is stored
  // in 'lepus_init_data_'.
  lepus::Value encoded_data;
  if (compile_options().enable_fiber_arch_) {
    encoded_data = lepus::Value::Clone(lepus_init_data_);
  } else {
    auto iter = page_moulds().find(0);
    encoded_data =
        (iter == page_moulds().end() ? lepus::Value() : iter->second->data());
  }

  return {GetName(),
          compile_options().target_sdk_version_,
          template_bundle_.support_component_js_,
          std::move(encoded_data),
          std::move(init_data_),
          std::move(cache_data_),
          GetJsBundle(),
          enable_circular_data_check_,
          enable_js_binding_api_throw_exception_,
          enable_bind_icu_,
          enable_microtask_promise_polyfill_,
          template_bundle().custom_sections_};
}

bool TemplateEntry::DecodeCSSFragmentById(int32_t fragmentId) {
  if (reader_) {
    return reader_->DecodeCSSFragmentByIdInRender(fragmentId);
  }
  return false;
}

bool TemplateEntry::LoadLepusChunk(const std::string& entry_path,
                                   const lepus::Value& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntry::LoadLepusChunk");

  LynxTemplateBundle& template_bundle = template_bundle_;

  auto lepus_chunk_opt = template_bundle.GetLepusChunk(entry_path);
  lepus::Value lepus_chunk_eval_result{};

  if (!lepus_chunk_opt) {
    if (reader_ && reader_->DecodeContextBundleInRender(entry_path)) {
      lepus_chunk_opt = template_bundle.GetLepusChunk(entry_path);
    }
  }

  if (lepus_chunk_opt) {
    GetVm()->DeSerialize(*lepus_chunk_opt->get(), true,
                         &lepus_chunk_eval_result,
                         GenerateLepusJSFileName(entry_path).c_str());
    return true;
  }
  return false;
}

std::unique_ptr<LynxBinaryRecyclerDelegate>
TemplateEntry::GetTemplateBundleRecycler() {
  return reader_ ? reader_->CreateRecycler() : nullptr;
}

fml::RefPtr<FiberElement> TemplateEntry::TryToGetElementCache() {
  fml::RefPtr<FiberElement> page_ref;
  const std::optional<LynxTemplateBundle>& template_bundle =
      GetCompleteTemplateBundle();
  if (template_bundle && template_bundle->GetContainsElementTree()) {
    auto& element_bundle = template_bundle->GetElementBundle();
    if (element_bundle.IsValid()) {
      page_ref = fml::static_ref_ptr_cast<FiberElement>(
          element_bundle.GetPageNode().RefCounted());
    }
  }
  return page_ref;
}

lepus::Value TemplateEntry::GetCustomSection(const std::string& key) {
  return template_bundle().GetCustomSection(key);
}

const std::string& TemplateEntry::GetErrorMsg() const { return error_msg_; }

void TemplateEntry::SetErrorMsg(std::string error_msg) {
  error_msg_ = std::move(error_msg);
}

}  // namespace tasm
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif
