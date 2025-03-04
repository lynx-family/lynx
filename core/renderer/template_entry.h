// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TEMPLATE_ENTRY_H_
#define CORE_RENDERER_TEMPLATE_ENTRY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/inspector/observer/inspector_lepus_observer.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/data/template_data.h"
#include "core/renderer/page_config.h"
#include "core/renderer/tasm_runtime_bundle.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/runtime/vm/lepus/lepus_global.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_lazy_reader_delegate.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
#include "core/template_bundle/template_codec/moulds.h"

namespace lynx {

namespace piper {
class NapiEnvironment;
}

namespace tasm {
class TemplateAssembler;
class ElementManager;
class FiberElement;

// base class released after dirived class,
// ensure vm context released after lepus value
class VmContextHolder {
 public:
  explicit VmContextHolder(const std::shared_ptr<lepus::Context> vm_context)
      : vm_context_(vm_context) {}
  virtual ~VmContextHolder() = default;

 protected:
  std::shared_ptr<lepus::Context> vm_context_;
};

class TemplateEntry : public VmContextHolder, public CSSStyleSheetDelegate {
 public:
  TemplateEntry();

  // Caller have to take the ownership for constructed object.
  static std::unique_ptr<TemplateEntry> ConstructEntryWithNoTemplateAssembler(
      std::shared_ptr<lepus::Context> context,
      const std::string& targetSdkVersion);

  bool InitWithTemplateBundle(
      TemplateBinaryReader::PageConfigger* page_configger,
      LynxTemplateBundle template_bundle);

  bool InitWithPageConfigger(
      TemplateBinaryReader::PageConfigger* page_configger);

  bool ConstructContext(TemplateAssembler* assembler, bool is_lepusng_binary,
                        const lepus::ContextBundle& context_bundle,
                        bool use_context_pool = false,
                        bool disable_tracing_gc = false);

  ~TemplateEntry() override;

  void SetLepusInitData(const lepus::Value& value) { lepus_init_data_ = value; }

  // Get, Only for use, Can't be stored.
  const std::shared_ptr<lepus::Context>& GetVm() const { return vm_context_; }
  const std::shared_ptr<CSSStyleSheetManager>& GetStyleSheetManager() {
    return template_bundle_.css_style_manager_;
  }
  piper::JsBundle& GetJsBundle() { return template_bundle_.GetJsBundle(); }
  std::unordered_map<int32_t, std::shared_ptr<PageMould>>& page_moulds() {
    return template_bundle_.page_moulds_;
  }
  std::unordered_map<int32_t, std::shared_ptr<DynamicComponentMould>>&
  lazy_bundle_moulds() {
    return template_bundle_.dynamic_component_moulds_;
  }

  std::unordered_map<int32_t, std::shared_ptr<ComponentMould>>&
  component_moulds() {
    return template_bundle_.component_moulds_;
  }

  inline const std::unordered_map<std::string, int>& component_name_to_id() {
    return template_bundle_.component_name_to_id_;
  }

  inline const std::unordered_map<std::string, std::string>
  lazy_bundle_declarations() {
    return template_bundle_.dynamic_component_declarations_;
  }

  std::string& GetName() { return name_; }

  inline std::optional<LynxTemplateBundle> GetCompleteTemplateBundle() {
    return is_template_bundle_complete_
               ? std::optional<LynxTemplateBundle>(template_bundle_)
               : std::nullopt;
  }

  void SetIsCard(bool is_card) { is_card_ = is_card; }

  const ElementTemplateInfo& GetElementTemplateInfo(const std::string& key);

  const std::shared_ptr<ParsedStyles>& GetParsedStyles(const std::string& key);

  const AirCompStylesMap& GetComponentParsedStyles(const std::string& path);

  void SetInitData(TemplateData init_data) {
    init_data_ = std::move(init_data);
  }

  // When enable data processor on JS thread, cache data will be set.
  void SetCacheData(std::vector<TemplateData> cache_data) {
    cache_data_ = std::move(cache_data);
  }

  void SetJSBundle(piper::JsBundle bundle) {
    template_bundle_.js_bundle_ = std::move(bundle);
  }
  void SetVm(std::shared_ptr<lepus::Context> vm) { vm_context_ = vm; }
  void SetName(const std::string& name);

  void AddLazyBundleDeclaration(const std::string& name,
                                const std::string& path);
  void InvokeLepusBridge(const int32_t callback_id, const lepus::Value& data);

  void ReInit(TemplateAssembler* assembler);

  piper::NapiEnvironment* napi_environment();
  void AttachNapiEnvironment();
  void DetachNapiEnvironment();

  bool IsCompatibleWithRootEntry(const TemplateEntry& root, std::string& msg);
  inline const tasm::CompileOptions& compile_options() const {
    return template_bundle_.compile_options_;
  }

  void MarkAsyncRendered() { need_async_render_ = false; }
  bool NeedAsyncRender() { return need_async_render_; }
  void SetCircularDataCheck(bool enable_check) {
    enable_circular_data_check_ = enable_check;
  }
  void SetTemplateDebugUrl(const std::string& debug_url) {
    template_bundle_.compile_options_.template_debug_url_ = debug_url;
  }

  const std::string& GetTemplateDebugUrl() const {
    return template_bundle_.compile_options_.template_debug_url_;
  }

  void SetLazyReader(std::unique_ptr<LynxBinaryLazyReaderDelegate> reader) {
    reader_ = std::move(reader);
  }

  void SetEnableJsBindingApiThrowException(bool enable) {
    enable_js_binding_api_throw_exception_ = enable;
  }

  void SetEnableBindICU(bool enable) { enable_bind_icu_ = enable; }
  void SetEnableMicrotaskPromisePolyfill(bool enable) {
    enable_microtask_promise_polyfill_ = enable;
  }

  LynxBinaryLazyReaderDelegate* GetReader() { return reader_.get(); }

  TasmRuntimeBundle CreateTasmRuntimeBundle();

  virtual bool DecodeCSSFragmentById(int32_t fragmentId) override;

  // Apply page_configs to LepusContext.
  void ApplyConfigsToLepusContext(
      const std::shared_ptr<PageConfig>& page_config);

  bool EnableReuseContext() {
    return template_bundle().ShouldReuseLepusContext();
  }

  // Execute Binary Function
  bool Execute();

  lepus::Value& component_info_map() { return component_info_map_; }

  lepus::Value& component_path_map() { return component_path_map_; }

  bool IsCard() const { return is_card_; }

  LynxTemplateBundle& template_bundle() { return template_bundle_; }

  lepus::Value GetBinaryEvalResult() { return binary_eval_result_; }

  lepus::Value ProcessBinaryEvalResult();

  bool LoadLepusChunk(const std::string& entry_path,
                      const lepus::Value& options);

  std::unique_ptr<LynxBinaryRecyclerDelegate> GetTemplateBundleRecycler();

  lepus::Value GetCustomSection(const std::string& key);

  explicit TemplateEntry(const std::shared_ptr<lepus::Context>& context,
                         const std::string& targetSdkVersion);

  fml::RefPtr<FiberElement> TryToGetElementCache();

  void SetLepusObserver(
      const std::shared_ptr<lepus::InspectorLepusObserver>& observer) {
    lepus_observer_ = observer;
  }

  // get error msg for initing template entry
  const std::string& GetErrorMsg() const;
  void SetErrorMsg(std::string error_msg);

 private:
  enum class LepusContextSourceType : int8_t {
    kFromRuntime = 0,
    kFromGlobalPool,
    kFromLocalPool,
  };

  void RegisterBuiltin(TemplateAssembler* assembler);

  void SetTemplateBundle(LynxTemplateBundle bundle);

  std::shared_ptr<PageConfig> EnsurePageConfig(
      TemplateBinaryReader::PageConfigger* configger) const;

  bool InitLepusContext(TemplateAssembler* tasm,
                        const std::shared_ptr<PageConfig>& page_config);

  std::string GenerateLepusJSFileName(const std::string& name);

  std::string name_;
  bool is_card_{true};

  // The variable 'lepus_init_data_' is applied for caching the initial data of
  // the top-level component/ card instance of the Lepus framework in fiber
  // mode, and synchronizing it with JS to prevent any potential breakage.
  lepus::Value lepus_init_data_{};

  // init_data passed from `LoadTemplate`, or result of `defaultDataProcessor`.
  // After executing CreateTasmRuntimeBundle, it will be moved.
  TemplateData init_data_;

  // Data pased from `UpdateTemplate` before exec `LoadTemplate`. After
  // executing CreateTasmRuntimeBundle, it will be moved.
  std::vector<TemplateData> cache_data_;

  bool need_async_render_ = true;
  bool enable_circular_data_check_ = true;

  std::weak_ptr<lepus::InspectorLepusObserver> lepus_observer_;
  std::unique_ptr<LynxBinaryLazyReaderDelegate> reader_;

  LynxTemplateBundle template_bundle_{};
  // whether the template bundle if from pre-decode, which is complte
  bool is_template_bundle_complete_{false};
  bool enable_js_binding_api_throw_exception_ = false;
  bool enable_bind_icu_ = false;
  bool enable_microtask_promise_polyfill_{false};
#if ENABLE_LEPUSNG_WORKLET
  std::unique_ptr<lynx::piper::NapiEnvironment> napi_environment_;
#endif

  // global maps usually used to create component or lazy bundle
  lepus::Value component_info_map_ = lepus::Value(lepus::Dictionary::Create());
  lepus::Value component_path_map_ = lepus::Value(lepus::Dictionary::Create());

  // result of entry's lepus.js.
  // now is only used for lazy bundle.
  lepus::Value binary_eval_result_{};

  // error msg for initing template entry
  std::string error_msg_{};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TEMPLATE_ENTRY_H_
