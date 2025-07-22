// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_LYNX_TEMPLATE_BUNDLE_H_
#define CORE_TEMPLATE_BUNDLE_LYNX_TEMPLATE_BUNDLE_H_

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/dom/element_bundle.h"
#include "core/renderer/template_themed.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/runtime/piper/js/js_bundle.h"
#include "core/runtime/vm/lepus/context_pool.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/template_bundle/template_codec/binary_decoder/parallel_parse_task_scheduler.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "core/template_bundle/template_codec/header_ext_info.h"
#include "core/template_bundle/template_codec/moulds.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {

namespace style {
class StyleObject;
}

namespace tasm {
class LepusChunkManager {
 public:
  using LepusChunkMap =
      std::unordered_map<std::string, std::shared_ptr<lepus::ContextBundle>>;

  std::optional<std::shared_ptr<lepus::ContextBundle>> GetLepusChunk(
      const std::string &chunk_key);
  bool IsLepusChunkDecoded(const std::string &chunk_path);
  void AddLepusChunk(const std::string &chunk_key,
                     std::shared_ptr<lepus::ContextBundle> bundle);

  std::atomic_bool GetStopThread() const { return stop_thread_; }
  void SetThreadStopFlag(bool stop_signal) { stop_thread_ = stop_signal; }

 private:
  LepusChunkMap lepus_chunk_map_{};
  std::unordered_set<std::string> decoded_lepus_chunks_{};

  volatile std::atomic_bool stop_thread_{false};
  std::mutex lepus_chunk_mutex_;

  friend class TemplateBinaryReader;
  friend class LynxBinaryReader;
};
// LynxTemplateBundle is used to hold the result of DecodeResult.
// It is usually used when user needs to decode a template without loading
// template.
//
class LynxTemplateBundle final {
 public:
  LynxTemplateBundle()
      : css_style_manager_(std::make_shared<CSSStyleSheetManager>(nullptr)),
        string_list_(std::make_shared<std::vector<base::String>>()),
        lepus_chunk_manager_(std::make_shared<LepusChunkManager>()){};

  const piper::JsBundle &GetJsBundle() const { return js_bundle_; }
  piper::JsBundle &GetJsBundle() { return js_bundle_; }

  inline std::optional<std::shared_ptr<lepus::ContextBundle>> GetLepusChunk(
      const std::string &chunk_key) const {
    return lepus_chunk_manager_->GetLepusChunk(chunk_key);
  }

  bool IsCard() const { return app_type_ == APP_TYPE_CARD; }
  bool GetContainsElementTree() const { return element_bundle_.IsValid(); };

  void SetCSSStyleManager(std::shared_ptr<CSSStyleSheetManager> manager) {
    css_style_manager_ = std::move(manager);
  }

  const std::shared_ptr<CSSStyleSheetManager> &GetCSSStyleManager() const {
    return css_style_manager_;
  }

  /**
   * @brief Initializes and returns a reference to the shared pointer of the
   * style object list.
   *
   * This method checks if the style object list has already been initialized.
   * If it has, it simply returns a reference to the existing list. Otherwise,
   * it creates a new array of pointers to `StyleObject` instances, initializes
   * the shared pointer with a custom deleter, and sets the last element of the
   * array to `nullptr` to mark the end of the list.
   *
   * @param size The size of the style object array to be created.
   * @return A reference to the shared pointer of the style object list. The
   * array is null` ended.
   */
  std::shared_ptr<style::StyleObject *> &InitStyleObjectList(size_t size);

  base::LinearFlatMap<base::String, fml::RefPtr<CSSKeyframesToken>> &
  InitKeyframesMap(size_t size);

  void SetLepusChunkManager(std::shared_ptr<LepusChunkManager> manager) {
    lepus_chunk_manager_ = std::move(manager);
  }

  const std::shared_ptr<LepusChunkManager> &GetLepusChunkManager() const {
    return lepus_chunk_manager_;
  }

  void SetElementBundle(ElementBundle element_bundle) {
    element_bundle_ = std::move(element_bundle);
  }

  const ElementBundle &GetElementBundle() const { return element_bundle_; }

  std::shared_ptr<std::vector<base::String>> &GetStringList() {
    return string_list_;
  }

  void SetStringList(
      const std::shared_ptr<std::vector<base::String>> &string_list) {
    string_list_ = string_list;
  }

  const std::vector<uint8_t> &GetBinary() const { return binary_; }
  void SetBinary(std::vector<uint8_t> binary) { binary_ = std::move(binary); }

  uint32_t Size() const { return total_size_; }

  bool is_lepusng_binary() { return is_lepusng_binary_; }

  bool ShouldReuseLepusContext() const;

  lepus::Value GetExtraInfo();

  // Calling to prepare lepusNg context by pageConfig or settings;
  // It should be called after decode success;
  void PrepareVMByConfigs();
  bool PrepareLepusContext(int32_t count);

  bool EnableUseContextPool() const { return force_use_context_pool_; }

  void SetEnableVMAutoGenerate(bool enable);

  void AddCustomSection(const std::string &key, const lepus::Value &value);

  lepus::Value GetCustomSection(const std::string &key);

  void GreedyConstructElements();

  bool EnableFiberArch() const { return compile_options_.enable_fiber_arch_; }

  std::optional<Elements> TryGetElements(const std::string &key);

  const std::shared_ptr<lynx::tasm::PageConfig> &GetPageConfig() {
    return page_configs_;
  };

 private:
  void EnsureParseTaskScheduler();

  // header info.
  uint32_t total_size_{0};
  bool is_lepusng_binary_{false};
  std::string lepus_version_{};
  std::string target_sdk_version_{};
  CompileOptions compile_options_{};
  lepus::Value template_info_{};
  bool enable_css_variable_{false};
  bool enable_css_parser_{false};
  bool support_component_js_{false};

  // app type.
  std::string app_type_{};

  // body - CSS
  std::shared_ptr<CSSStyleSheetManager> css_style_manager_;

  // body - StyleObjects

  std::shared_ptr<style::StyleObject *> style_object_list_{nullptr};

  CSSKeyframesTokenMap keyframes_;

  // body - APP
  std::string app_name_;

  // body - PAGE
  std::unordered_map<int32_t, std::shared_ptr<PageMould>> page_moulds_{};

  // body - String
  std::shared_ptr<std::vector<base::String>> string_list_{};

  // body - COMPONENT
  std::unordered_map<std::string, int32_t> component_name_to_id_{};
  std::unordered_map<int32_t, std::shared_ptr<ComponentMould>>
      component_moulds_{};

  // body - JS
  piper::JsBundle js_bundle_{};

  // body - CONFIG
  std::shared_ptr<lynx::tasm::PageConfig> page_configs_{};

  // body - DYNAMIC-COMPONENT
  std::unordered_map<int32_t, std::shared_ptr<DynamicComponentMould>>
      dynamic_component_moulds_;

  // body - THEMED.
  Themed themed_;

  // body - USING_DYNAMIC_COMPONENT_INFO
  std::unordered_map<std::string, std::string>
      dynamic_component_declarations_{};

  // body - lepus context binary
  std::shared_ptr<lepus::ContextBundle> context_bundle_{nullptr};
  std::shared_ptr<lepus::LynxContextPool> context_pool_{nullptr};

  // fiber - lepus chunk binary
  std::shared_ptr<LepusChunkManager> lepus_chunk_manager_;

  // fiber- element template info map
  std::unordered_map<std::string, std::shared_ptr<ElementTemplateInfo>>
      element_template_infos_{};

  // fiber- parsed styles map
  ParsedStylesMap parsed_styles_map_;

  // air parsed styles
  AirParsedStylesMap air_parsed_styles_map_;

  // This field stores the original binary, which is only recorded when devtool
  // is enabled, and is only used by devtool
  std::vector<uint8_t> binary_;

  ElementBundle element_bundle_;

  // force to use context pool in runtime
  bool force_use_context_pool_{false};

  lepus::Value custom_sections_{};

  // timing
  uint64_t decode_start_timestamp_{0};
  uint64_t decode_end_timestamp_{0};

  std::shared_ptr<ParallelParseTaskScheduler> task_schedular_{nullptr};

  friend class LynxBinaryReader;
  friend class TemplateAssembler;
  friend class TemplateEntry;
  friend class LynxTemplateBundleConverter;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_LYNX_TEMPLATE_BUNDLE_H_
