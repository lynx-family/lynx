// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PAGE_CONFIG_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PAGE_CONFIG_H_

#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_config_auto_gen.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "core/template_bundle/template_codec/ttml_constant.h"
#include "core/template_bundle/template_codec/version.h"

namespace lynx {
namespace tasm {
// Preallocate 64 bit unsigned integer for pipeline scheduler config.
// 0 ~ 7 bit: Reserved for parsing binary bundle into C++ bundle.
// 8 ~ 15 bit: Reserved for MTS Render.
// 16 ~ 23 bit: Reserved for resolve stage in Pixel Pipeline.
// 24 ~ 31 bit: Reserved for layout stage in Pixel Pipeline.
// 32 ~ 39 bit: Reserved for execute UI OP stage in Pixel Pipeline.
// 40 ~ 47 bit: Reserved for paint stage in Pixel Pipeline.
// 48 ~ 63 bit: Flexible bits for extensibility.
// Use 2-bits for each feature flag to represent three states:
// true/false/undefine 00: represents undefine 01: represents enable feature
// flag 10: represents disable feature flag
// TODO: Need to add a TS definition for PipelineSchedulerConfig.
constexpr static uint64_t kEnableParallelParseElementTemplate = 1;
constexpr static uint64_t kEnableListBatchRenderMask = 1 << 8;
constexpr static uint64_t kEnableParallelElementMask = 1 << 16;
constexpr static uint64_t kDisableParallelElementMask = 1 << 17;
constexpr static uint64_t kEnableListBatchRenderAsyncResolvePropertyMask =
    1 << 18;
constexpr static uint64_t kEnableListBatchRenderAsyncResolveTreeMask = 1 << 20;
constexpr static uint64_t kEnableParallelElementLevelOrderMask = 1 << 22;
constexpr static uint64_t kDisableParallelElementLevelOrderMask = 1 << 23;

// TODO(nihao.royal) unify parameters of different types.
// constexpr static int32_t kTernaryInt32UndefinedValue = 0x7fffffff;
// constexpr static const char* kTernaryStringUndefinedValue = "undefined";

/**
 * PageConfig hold overall configs of a page.
 * When adding or modifying some properties, please modify
 * oliver/type-lynx/compile/page-config.d.ts at the same time.
 */
class PageConfig final : public LynxConfig {
 public:
  // Enable attribute flatten if it not defined in index.json
  // Attribute auto-expose is automatically opended
  PageConfig()
      : bundle_module_mode_(PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE),
        dsl_(PackageInstanceDSL::TT){};

  ~PageConfig() override = default;

  std::unordered_map<std::string, std::string> GetPageConfigMap() {
    std::unordered_map<std::string, std::string> map;
    map.insert({"page_flatten", page_flatten_ ? "true" : "false"});
    map.insert({"target_sdk_version", target_sdk_version_});
    map.insert({"enable_lepus_ng", enable_lepus_ng_ ? "true" : "false"});
    map.insert({"react_version", react_version_});
    map.insert({"enable_css_parser", enable_css_parser_ ? "true" : "false"});
    map.insert({"absetting_disable_css_lazy_decode",
                absetting_disable_css_lazy_decode_});
    return map;
  }

  inline void SetOriginalConfig(std::string config_str) {
    original_config_ = std::move(config_str);
  }

  inline std::string GetOriginalConfig() { return original_config_; }

  inline void SetDSL(PackageInstanceDSL dsl) { dsl_ = dsl; }

  inline void SetQuirksMode(bool enable) {
    if (css_align_with_legacy_w3c_ || !enable) {
      layout_configs_.SetQuirksMode(kQuirksModeDisableVersion);
    } else {
      layout_configs_.SetQuirksMode(kQuirksModeEnableVersion);
    }
  }
  inline bool GetQuirksMode() const {
    return layout_configs_.IsFullQuirksMode();
  }

  inline void SetQuirksModeByVersion(const base::Version& version) {
    if (css_align_with_legacy_w3c_) {
      layout_configs_.SetQuirksMode(kQuirksModeDisableVersion);
    } else {
      layout_configs_.SetQuirksMode(version);
    }
  }
  inline base::Version GetQuirksModeVersion() const {
    return layout_configs_.GetQuirksMode();
  }

  inline void SetDefaultOverflowVisible(bool is_visible) {
    default_overflow_visible_ = is_visible;
  }

  inline bool GetDefaultOverflowVisible() { return default_overflow_visible_; }

  inline const tasm::DynamicCSSConfigs& GetDynamicCSSConfigs() {
    return css_configs_;
  }

  inline PackageInstanceDSL GetDSL() { return dsl_; }

  inline void SetBundleModuleMode(
      PackageInstanceBundleModuleMode bundle_module_mode) {
    bundle_module_mode_ = bundle_module_mode;
  }

  inline PackageInstanceBundleModuleMode GetBundleModuleMode() {
    return bundle_module_mode_;
  }

  bool GetCSSAlignWithLegacyW3C() const { return css_align_with_legacy_w3c_; }
  void SetCSSAlignWithLegacyW3C(bool val) {
    css_align_with_legacy_w3c_ = val;
    layout_configs_.css_align_with_legacy_w3c_ = val;
    if (val) {
      layout_configs_.SetQuirksMode(kQuirksModeDisableVersion);
    }
  }

  // TODO(liting.src): just a workaround to leave below APIs for ssr
  bool GetEnableLocalAsset() const { return false; }
  void SetEnableLocalAsset(bool val) {}

  inline const CSSParserConfigs& GetCSSParserConfigs() {
    return css_parser_configs_;
  }

  void SetCSSParserConfigs(const CSSParserConfigs& config) {
    css_parser_configs_ = config;
  }

  inline void SetTargetSDKVersion(const std::string& target_sdk_version) {
    target_sdk_version_ = target_sdk_version;
    layout_configs_.SetTargetSDKVersion(target_sdk_version);
    SetIsTargetSdkVerionHigherThan21();
  }
  inline std::string GetTargetSDKVersion() { return target_sdk_version_; }

  inline void SetIsTargetSdkVerionHigherThan21() {
    is_target_sdk_verion_higher_than_2_1_ =
        lynx::base::Version(target_sdk_version_) >
        lynx::base::Version(LYNX_VERSION_2_1);
  }

  inline void SetIsTargetSdkVerionHigherThan21(bool value) {
    is_target_sdk_verion_higher_than_2_1_ = value;
  }

  inline bool GetIsTargetSdkVerionHigherThan21() const {
    return is_target_sdk_verion_higher_than_2_1_;
  }

  inline void SetLepusVersion(const std::string& lepus_version) {
    lepus_version_ = lepus_version;
  }
  inline std::string GetLepusVersion() { return lepus_version_; }

  inline void SetEnableLepusNG(bool enable_lepus_ng) {
    enable_lepus_ng_ = enable_lepus_ng;
  }
  inline bool GetEnableLepusNG() { return enable_lepus_ng_; }

  void SetEnableSavePageData(bool enable) { enable_save_page_data_ = enable; }

  bool GetEnableSavePageData() { return enable_save_page_data_; }

  void SetListRemoveComponent(bool list_remove_component) {
    list_remove_component_ = list_remove_component;
  }
  bool GetListRemoveComponent() { return list_remove_component_; }

  inline bool GetEnableZIndex() { return enable_z_index_; }
  inline void SetEnableZIndex(bool enable) { enable_z_index_ = enable; }

  inline bool GetEnableLynxAir() { return enable_lynx_air_; }
  inline void SetEnableLynxAir(bool enable) { enable_lynx_air_ = enable; }
  inline bool GetEnableFiberArch() { return enable_fiber_arch_; }
  inline void SetEnableFiberArch(bool enable) { enable_fiber_arch_ = enable; }

  inline bool GetEnableCSSParser() { return enable_css_parser_; }
  inline void SetEnableCSSParser(bool enable) { enable_css_parser_ = enable; }

  inline std::string GetAbSettingDisableCSSLazyDecode() {
    return absetting_disable_css_lazy_decode_;
  }
  inline void SetAbSettingDisableCSSLazyDecode(std::string disable) {
    absetting_disable_css_lazy_decode_ = disable;
  }

  inline void SetEnableEventRefactor(bool option) {
    enable_event_refactor_ = option;
  }

  bool GetEnableEventRefactor() const { return enable_event_refactor_; }

  int32_t GetIncludeFontPadding() const { return include_font_padding_; }

  void SetIncludeFontPadding(bool value) {
    include_font_padding_ = value ? 1 : -1;
  }

  inline void SetLynxAirMode(CompileOptionAirMode air_mode) {
    air_mode_ = air_mode;
  }

  inline CompileOptionAirMode GetLynxAirMode() { return air_mode_; }

  inline bool GetEnableRasterAnimation() const {
    return enable_raster_animation_;
  }
  inline void SetEnableRasterAnimation(bool value) {
    enable_raster_animation_ = value;
  }

  inline bool GetEnableCSSInvalidation() const {
    return enable_css_invalidation_;
  }

  inline void SetEnableCSSInvalidation(bool enable) {
    enable_css_invalidation_ = enable;
  }

  inline bool GetEnableParallelParseElementTemplate() {
    return pipeline_scheduler_config_ & kEnableParallelParseElementTemplate;
  }

  bool GetEnableParallelElement() const;

  inline void SetEnableParallelElement(bool enable) {
    enable_parallel_element_ = enable;
  }

  bool GetEnableStandardCSSSelector() const {
    return enable_standard_css_selector_;
  }

  void SetEnableStandardCSSSelector(bool enable) {
    enable_standard_css_selector_ = enable;
  }

  bool GetEnableComponentAsyncDecode() const {
    switch (enable_component_async_decode_) {
      case TernaryBool::TRUE_VALUE:
        return true;
      case TernaryBool::FALSE_VALUE:
        return false;
      case TernaryBool::UNDEFINE_VALUE:
        static bool enable_from_experiment =
            LynxEnv::GetInstance().EnableComponentAsyncDecode();
        return enable_from_experiment;
    }
  }

  bool GetEnableUseContextPool() const {
    switch (enable_use_context_pool_) {
      case TernaryBool::TRUE_VALUE:
        return true;
      case TernaryBool::FALSE_VALUE:
        return false;
      case TernaryBool::UNDEFINE_VALUE:
        static bool enable_from_experiment =
            LynxEnv::GetInstance().EnableUseContextPool();
        return enable_from_experiment;
    }
  }

  inline void SetEnableScrollFluencyMonitor(double value) {
    if (value < 0) {
      enable_scroll_fluency_monitor_ = 0;
    } else if (value > 1) {
      enable_scroll_fluency_monitor_ = 1;
    } else {
      enable_scroll_fluency_monitor_ = value;
    }
  }
  inline double GetEnableScrollFluencyMonitor() {
    return enable_scroll_fluency_monitor_;
  }

  inline bool GetEnableCSSLazyImport() const {
    // pageConfig > Libra > Settings
    switch (enable_css_lazy_import_) {
      case TernaryBool::TRUE_VALUE:
        return true;
      case TernaryBool::FALSE_VALUE:
        return false;
      case TernaryBool::UNDEFINE_VALUE:
        static bool enable_css_lazy_import =
            LynxEnv::GetInstance().EnableCSSLazyImport();
        return enable_css_lazy_import;
    }
  }

  inline bool GetEnableNewAnimator() const {
    // pageConfig > Libra > Settings
    switch (enable_new_animator_) {
      case TernaryBool::TRUE_VALUE:
        return true;
      case TernaryBool::FALSE_VALUE:
        return false;
      case TernaryBool::UNDEFINE_VALUE:
        static bool enable_new_animator =
            LynxEnv::GetInstance().EnableNewAnimatorFiber();
        return enable_new_animator;
    }
  }

  bool GetEnableNativeScheduleCreateViewAsyncAsBool() const {
    if (enable_native_schedule_create_view_async_ ==
        TernaryBool::UNDEFINE_VALUE) {
      return LynxEnv::GetInstance().EnableNativeCreateViewAsync();
    } else {
      return enable_native_schedule_create_view_async_ ==
             TernaryBool::TRUE_VALUE;
    }
  }

  bool GetEnableSignalAPIBoolValue() {
    if (enable_signal_api_ == TernaryBool::UNDEFINE_VALUE) {
      enable_signal_api_ = LynxEnv::GetInstance().EnableSignalAPI()
                               ? TernaryBool::TRUE_VALUE
                               : TernaryBool::UNDEFINE_VALUE;
    }
    return enable_signal_api_ == TernaryBool::TRUE_VALUE;
  }

  // TODO(songshourui.null): move this function to testing file
  void PrintPageConfig(std::ostream& output) {
#define PAGE_CONFIG_DUMP(key) output << #key << ":" << key << ",";
    output << "page_version:" << page_version_ << ",";
    output << "page_flatten:" << page_flatten_ << ",";
    output << "page_implicit:" << page_implicit_ << ",";
    output << "dsl_:" << static_cast<int>(dsl_) << ",";
    output << "enable_auto_show_hide:" << enable_auto_show_hide_ << ",";
    output << "bundle_module_mode_:" << static_cast<int>(bundle_module_mode_)
           << ",";
    PAGE_CONFIG_DUMP(enable_async_display_)
    PAGE_CONFIG_DUMP(enable_view_receive_touch_)
    PAGE_CONFIG_DUMP(enable_lepus_strict_check_)
    PAGE_CONFIG_DUMP(enable_event_through_)
    PAGE_CONFIG_DUMP(layout_configs_.is_absolute_in_content_bound_)
    output << "layout_configs_.quirks_mode_:"
           << layout_configs_.IsFullQuirksMode() << ",";
    PAGE_CONFIG_DUMP(css_parser_configs_.enable_css_strict_mode)
#undef PAGE_CONFIG_DUMP
  }

  // TODO(songshourui.null): move this function to testing file
  std::string StringifyPageConfig() {
    std::ostringstream output;
    PrintPageConfig(output);
    return output.str();
  }

  bool NeedPostToPlatform() const { return need_post_to_platform_; }

  // TODO(zhoupeng.z): remove this method after pre-postings applied on all
  // platforms.
  void MarkPostToPlatform() { need_post_to_platform_ = false; }

  bool GetEnableLevelOrderTraversing();

 private:
  std::string target_sdk_version_;
  std::string lepus_version_;
  std::string absetting_disable_css_lazy_decode_;
  std::string original_config_{};
  PackageInstanceBundleModuleMode bundle_module_mode_;
  // default include font padding
  // 1 means true
  // -1 means false
  int32_t include_font_padding_{0};
  CSSParserConfigs css_parser_configs_;
  PackageInstanceDSL dsl_;
  bool enable_lepus_ng_{true};
  bool default_overflow_visible_{false};
  bool enable_save_page_data_{false};
  bool list_remove_component_{false};
  bool enable_z_index_{false};
  bool enable_lynx_air_{false};
  bool enable_fiber_arch_{false};
  // Used for lynx config
  bool enable_css_parser_{false};
  bool is_target_sdk_verion_higher_than_2_1_{false};
  bool enable_event_refactor_{true};

  CompileOptionAirMode air_mode_{CompileOptionAirMode::AIR_MODE_OFF};
  // set text overflow as visible if true

  // support CSS invalidation
  bool enable_css_invalidation_{false};

  // indicate that enable standard css selector
  bool enable_standard_css_selector_{false};

  // Indicates whether the parallel flush of Element has been enabled. And the
  // default value is false.
  bool enable_parallel_element_{false};

  // enable raster animation
  bool enable_raster_animation_{false};

  TernaryBool enable_level_order_traversing_{TernaryBool::UNDEFINE_VALUE};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PAGE_CONFIG_H_
