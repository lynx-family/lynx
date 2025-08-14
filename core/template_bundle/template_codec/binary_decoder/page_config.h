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
#include "core/renderer/css/dynamic_css_configs.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/starlight/types/layout_configs.h"
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

// TODO(nihao.royal) unify parameters of different types.
// constexpr static int32_t kTernaryInt32UndefinedValue = 0x7fffffff;
// constexpr static const char* kTernaryStringUndefinedValue = "undefined";

static constexpr const char kEnableSignalAPI[] = "enableSignalAPI";
static constexpr const char kEnableNativeScheduleCreateViewAsync[] =
    "enableNativeScheduleCreateViewAsync";

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
        dsl_(PackageInstanceDSL::TT),
        enable_auto_show_hide(true){};

  ~PageConfig() override = default;

  void DecodePageConfigFromJsonStringWhileUndefined(
      const std::string& config_json_string) {
    rapidjson::Document doc;

    if (!doc.Parse(config_json_string.c_str()).HasParseError()) {
      for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
        const char* const name = it->name.GetString();

        // if is a boolean
        auto bool_pair = GetFuncBoolMap().find(name);
        if (bool_pair != GetFuncBoolMap().end()) {
          if (it->value.IsBool()) {
            const auto [setter, getter] = bool_pair->second;
            if ((this->*(getter))() == TernaryBool::UNDEFINE_VALUE) {
              (this->*(setter))(it->value.GetBool() ? TernaryBool::TRUE_VALUE
                                                    : TernaryBool::FALSE_VALUE);
              need_post_to_platform_ = true;
            }
          }
        }

        // if is a uint64
        auto uint64_pair = GetFuncUint64Map().find(name);
        if (uint64_pair != GetFuncUint64Map().end()) {
          if (it->value.IsUint64()) {
            const auto [setter, getter] = uint64_pair->second;
            if ((this->*(getter))() == 0) {
              (this->*(setter))(it->value.GetUint64());
              need_post_to_platform_ = true;
            }
          }
        }

        // TODO(nihao.royal) unify parameters of different types.
      }
    }
  }

  void ForEachBoolConfig(
      const base::MoveOnlyClosure<TernaryBool, const std::string&> func) {
    for (const auto& [name, pair] : GetFuncBoolMap()) {
      const auto [setter, getter] = pair;
      (this->*(setter))(func(name));
    }
  }

  void ForEachUint64Config(
      const base::MoveOnlyClosure<uint64_t, const std::string&> func) {
    for (const auto& [name, pair] : GetFuncUint64Map()) {
      const auto [setter, getter] = pair;
      (this->*(setter))(func(name));
    }
  }

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

  inline void SetEnableA11yIDMutationObserver(bool enable) {
    enable_a11y_mutation_observer = enable;
  }

  inline void SetEnableA11y(bool enable) { enable_a11y = enable; }

  inline bool GetEnableA11yIDMutationObserver() {
    return enable_a11y_mutation_observer;
  }

  inline bool GetEnableA11y() { return enable_a11y; }

  inline void SetDSL(PackageInstanceDSL dsl) { dsl_ = dsl; }

  inline void SetAutoExpose(bool enable) { enable_auto_show_hide = enable; }

  inline void SetAbsoluteInContentBound(bool enable) {
    layout_configs_.is_absolute_in_content_bound_ = enable;
  }

  inline bool GetAbsoluteInContentBound() {
    return layout_configs_.is_absolute_in_content_bound_;
  }

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

  inline bool GetAutoExpose() { return enable_auto_show_hide; }

  inline void SetDefaultOverflowVisible(bool is_visible) {
    default_overflow_visible_ = is_visible;
  }

  inline bool GetDefaultOverflowVisible() { return default_overflow_visible_; }

  inline const tasm::DynamicCSSConfigs& GetDynamicCSSConfigs() {
    return css_configs_;
  }

  inline void SetEnableFixedNew(bool enable) {
    layout_configs_.enable_fixed_new_ = enable;
  }
  inline bool GetEnableFixedNew() const {
    return layout_configs_.enable_fixed_new_;
  }

  inline PackageInstanceDSL GetDSL() { return dsl_; }

  inline void SetBundleModuleMode(
      PackageInstanceBundleModuleMode bundle_module_mode) {
    bundle_module_mode_ = bundle_module_mode;
  }

  inline PackageInstanceBundleModuleMode GetBundleModuleMode() {
    return bundle_module_mode_;
  }

  inline void SetTrailNewImage(TernaryBool enable) {
    trail_New_Image_ = enable;
  }

  inline TernaryBool GetTrailNewImage() const { return trail_New_Image_; }

  inline void SetEnableTextLanguageAlignment(bool enable) {
    enable_text_language_alignment_ = enable;
  }

  inline bool GetEnableTextLanguageAlignment() {
    return enable_text_language_alignment_;
  }
  inline void SetEnableXTextLayoutReused(bool enable) {
    enable_x_text_layout_reused_ = enable;
  }
  inline bool GetEnableXTextLayoutReused() {
    return enable_x_text_layout_reused_;
  }

  inline void SetFontScaleSpOnly(bool font_scale) {
    layout_configs_.font_scale_sp_only_ = font_scale;
  }

  inline bool GetFontScaleSpOnly() {
    return layout_configs_.font_scale_sp_only_;
  }

  void SetEnableTouchRefactor(bool enable) { enable_touch_refactor_ = enable; }

  bool GetEnableTouchRefactor() { return enable_touch_refactor_; }

  void SetEnableEndGestureAtLastFingerUp(bool enable) {
    enable_end_gesture_at_last_finger_up_ = enable;
  }

  bool GetEnableEndGestureAtLastFingerUp() {
    return enable_end_gesture_at_last_finger_up_;
  }

  void SetEnableCSSInheritance(bool enable) {
    css_configs_.enable_css_inheritance_ = enable;
  }

  bool GetEnableCSSInheritance() {
    return css_configs_.enable_css_inheritance_;
  }

  void SetCustomCSSInheritList(std::unordered_set<CSSPropertyID>&& list) {
    css_configs_.custom_inherit_list_ =
        std::forward<std::unordered_set<CSSPropertyID>>(list);
  }

  const std::unordered_set<CSSPropertyID>& GetCustomCSSInheritList() {
    return css_configs_.custom_inherit_list_;
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

  void SetAsyncRedirectUrl(TernaryBool async) { async_redirect_url = async; }
  TernaryBool GetAsyncRedirectUrl() const { return async_redirect_url; }

  void SetEnableCSSStrictMode(bool enable) {
    css_parser_configs_.enable_css_strict_mode = enable;
  }

  bool GetEnableCSSStrictMode() {
    return css_parser_configs_.enable_css_strict_mode;
  }

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

  void SetUnifyVWVH(bool unify) { css_configs_.unify_vw_vh_behavior_ = unify; }
  bool GetUnifyVWVH() { return css_configs_.unify_vw_vh_behavior_; }

  inline bool GetEnableZIndex() { return enable_z_index_; }
  inline void SetEnableZIndex(bool enable) { enable_z_index_ = enable; }

  inline bool GetEnableRemoveComponentExtraData() const {
    return enable_remove_component_extra_data_;
  }
  inline void SetEnableRemoveComponentExtraData(bool enable) {
    enable_remove_component_extra_data_ = enable;
  }

  inline bool GetEnableLynxAir() { return enable_lynx_air_; }
  inline void SetEnableLynxAir(bool enable) { enable_lynx_air_ = enable; }
  inline bool GetEnableFiberArch() { return enable_fiber_arch_; }
  inline void SetEnableFiberArch(bool enable) { enable_fiber_arch_ = enable; }

  void SetEnableTextLayoutCache(TernaryBool enable_text_layout_cache) {
    enable_text_layout_cache_ = enable_text_layout_cache;
  }

  inline TernaryBool GetEnableTextLayoutCache() {
    return enable_text_layout_cache_;
  }

  void SetEnableUnifiedPipeline(TernaryBool enable_unified_pipeline) {
    enable_unified_pipeline_ = enable_unified_pipeline;
  }

  TernaryBool GetEnableUnifiedPipeline() const {
    return enable_unified_pipeline_;
  }

  inline bool GetEnableCSSParser() { return enable_css_parser_; }
  inline void SetEnableCSSParser(bool enable) { enable_css_parser_ = enable; }

  inline std::string GetAbSettingDisableCSSLazyDecode() {
    return absetting_disable_css_lazy_decode_;
  }
  inline void SetAbSettingDisableCSSLazyDecode(std::string disable) {
    absetting_disable_css_lazy_decode_ = disable;
  }

  inline void SetKeyboardCallbackUseRelativeHeight(bool enable) {
    keyboard_callback_pass_relative_height_ = enable;
  }

  inline bool GetKeyboardCallbackUseRelativeHeight() const {
    return keyboard_callback_pass_relative_height_;
  }

  inline void SetEnableEventRefactor(bool option) {
    enable_event_refactor_ = option;
  }

  bool GetEnableEventRefactor() const { return enable_event_refactor_; }

  inline void SetForceCalcNewStyle(bool option) {
    force_calc_new_style_ = option;
  }

  bool GetForceCalcNewStyle() const { return force_calc_new_style_; }

  inline void SetCompileRender(bool option) { compile_render_ = option; }

  bool GetCompileRender() const { return compile_render_; }

  inline void SetDisableLongpressAfterScroll(bool value) {
    disable_longpress_after_scroll_ = value;
  }

  inline bool GetDisableLongpressAfterScroll() {
    return disable_longpress_after_scroll_;
  }

  inline void SetEnableCheckDataWhenUpdatePage(bool option) {
    enable_check_data_when_update_page_ = option;
  }

  bool GetEnableCheckDataWhenUpdatePage() const {
    return enable_check_data_when_update_page_;
  }

  int32_t GetIncludeFontPadding() const { return include_font_padding_; }

  void SetIncludeFontPadding(bool value) {
    include_font_padding_ = value ? 1 : -1;
  }

  inline void SetEnableNewIntersectionObserver(bool option) {
    enable_new_intersection_observer_ = option;
  }

  inline bool GetEnableNewIntersectionObserver() const {
    return enable_new_intersection_observer_;
  }

  inline void SetObserverFrameRate(int32_t option) {
    observer_frame_rate_ = option;
  }

  inline int32_t GetObserverFrameRate() const { return observer_frame_rate_; }

  inline void SetEnableCheckExposureOptimize(
      bool enable_check_exposure_optimize) {
    enable_check_exposure_optimize_ = enable_check_exposure_optimize;
  }

  inline bool GetEnableCheckExposureOptimize() const {
    return enable_check_exposure_optimize_;
  }

  inline void SetEnableDisexposureWhenLynxHidden(
      bool enable_disexposure_when_lynx_hidden) {
    enable_disexposure_when_lynx_hidden_ = enable_disexposure_when_lynx_hidden;
  }

  inline bool GetEnableDisexposureWhenLynxHidden() const {
    return enable_disexposure_when_lynx_hidden_;
  }

  inline void SetEnableExposureWhenLayout(bool value) {
    enable_exposure_when_layout_ = value;
  }

  inline bool GetEnableExposureWhenLayout() const {
    return enable_exposure_when_layout_;
  }

  inline void SetEnableAirDetectRemovedKeysWhenUpdateData(bool value) {
    enable_air_detect_removed_keys_when_update_data_ = value;
  }
  inline bool GetEnableAirDetectRemovedKeysWhenUpdateData() const {
    return enable_air_detect_removed_keys_when_update_data_;
  }

  inline void SetEnableExposureUIMargin(bool option) {
    enable_exposure_ui_margin_ = option;
  }

  inline bool GetEnableExposureUIMargin() const {
    return enable_exposure_ui_margin_;
  }

  inline void SetEnableNewGesture(bool enable) { enable_new_gesture_ = enable; }

  inline bool GetEnableNewGesture() const { return enable_new_gesture_; }

  inline void SetLongPressDuration(int32_t option) {
    long_press_duration_ = option;
  }

  inline void SetMapContainerType(uint8_t type) { map_container_type_ = type; }

  inline uint8_t GetMapContainerType() { return map_container_type_; }

  inline int32_t GetLongPressDuration() const { return long_press_duration_; }

  inline void SetEnableCheckLocalImage(bool option) {
    enable_check_local_image_ = option;
  }

  inline bool GetEnableCheckLocalImage() const {
    return enable_check_local_image_;
  }

  inline void SetEnableAsyncRequestImage(bool option) {
    enable_async_request_image_ = option;
  }

  inline bool GetEnableAsyncRequestImage() const {
    return enable_async_request_image_;
  }

  inline void SetEnableBackgroundShapeLayer(bool enable) {
    enable_background_shape_layer_ = enable;
  }

  inline bool GetEnableBackgroundShapeLayer() {
    return enable_background_shape_layer_;
  }

  inline void SetLynxAirMode(CompileOptionAirMode air_mode) {
    air_mode_ = air_mode;
  }

  inline CompileOptionAirMode GetLynxAirMode() { return air_mode_; }

  inline bool GetEnableCascadePseudo() const { return enable_cascade_pseudo_; }
  inline void SetEnableCascadePseudo(bool value) {
    enable_cascade_pseudo_ = value;
  }

  inline bool GetEnableRasterAnimation() const {
    return enable_raster_animation_;
  }
  inline void SetEnableRasterAnimation(bool value) {
    enable_raster_animation_ = value;
  }

  inline lepus::Value GetExtraInfo() const { return extra_info_; }

  inline void SetExtraInfo(lepus::Value extra_info) {
    extra_info_ = extra_info;
  }

  int64_t GetLepusGCThreshold() { return lepus_gc_threshold_; }
  void SetLepusGCThreshold(int64_t value) { lepus_gc_threshold_ = value; }

  inline bool GetEnableComponentNullProp() const {
    return enable_component_null_prop_;
  }

  inline void SetEnableComponentNullProp(bool enable_component_null_prop) {
    enable_component_null_prop_ = enable_component_null_prop;
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

  inline uint64_t GetPipelineSchedulerConfig() const {
    return pipeline_scheduler_config_;
  }

  inline void SetPipelineSchedulerConfig(uint64_t config) {
    pipeline_scheduler_config_ = config;
  }

  bool GetRemoveDescendantSelectorScope() const {
    return remove_descendant_selector_scope_;
  }

  void SetRemoveDescendantSelectorScope(bool enable) {
    remove_descendant_selector_scope_ = enable;
  }

  bool GetEnableStandardCSSSelector() const {
    return enable_standard_css_selector_;
  }

  void SetEnableStandardCSSSelector(bool enable) {
    enable_standard_css_selector_ = enable;
  }

  bool GetEnableDataProcessorOnJs() const {
    return enable_data_processor_on_js_;
  }

  void SetEnableDataProcessorOnJs(bool enable) {
    enable_data_processor_on_js_ = enable;
  }

  inline TernaryBool GetEnableNativeList() const { return enable_native_list_; }

  inline void SetEnableNativeList(TernaryBool enable) {
    enable_native_list_ = enable;
  }

  bool GetEnableMultiTouch() const { return enable_multi_touch_; }

  void SetEnableMultiTouch(bool enable) { enable_multi_touch_ = enable; }

  bool GetEnableMultiTouchParamsCompatible() const {
    return enable_multi_touch_params_compatible_;
  }

  void SetEnableMultiTouchParamsCompatible(bool enable) {
    enable_multi_touch_params_compatible_ = enable;
  }

  bool GetEnableHarmonyVisibleAreaChangeForExposure() const {
    return enable_harmony_visible_area_change_for_exposure_;
  }

  void SetEnableHarmonyVisibleAreaChangeForExposure(bool enable) {
    enable_harmony_visible_area_change_for_exposure_ = enable;
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

  void SetEnableComponentAsyncDecode(bool enable) {
    enable_component_async_decode_ =
        enable ? TernaryBool::TRUE_VALUE : TernaryBool::FALSE_VALUE;
  }

  void SetEnableUseContextPool(bool enable) {
    enable_use_context_pool_ =
        enable ? TernaryBool::TRUE_VALUE : TernaryBool::FALSE_VALUE;
  }

  inline void SetEnableAsyncResolveSubtree(bool enable) {
    enable_async_resolve_subtree_ =
        enable ? TernaryBool::TRUE_VALUE : TernaryBool::FALSE_VALUE;
  }

  inline bool GetEnableAsyncResolveSubtree() {
    return enable_async_resolve_subtree_ == TernaryBool::TRUE_VALUE;
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
      enable_scroll_fluency_monitor = 0;
    } else if (value > 1) {
      enable_scroll_fluency_monitor = 1;
    } else {
      enable_scroll_fluency_monitor = value;
    }
  }
  inline double GetEnableScrollFluencyMonitor() {
    return enable_scroll_fluency_monitor;
  }

  bool GetEnableJsBindingApiThrowException() const {
    return enable_js_binding_api_throw_exception_;
  }

  void SetEnableJsBindingApiThrowException(bool enable) {
    enable_js_binding_api_throw_exception_ = enable;
  }

  void SetEnableUseMapBuffer(TernaryBool use_map_buffer) {
    enable_use_map_buffer_ = use_map_buffer;
  }

  TernaryBool GetEnableUseMapBuffer() const { return enable_use_map_buffer_; }

  void SetEnableUIOperationOptimize(TernaryBool enable) {
    enable_ui_operation_optimize_ = enable;
  }

  TernaryBool GetEnableUIOperationOptimize() const {
    return enable_ui_operation_optimize_;
  }

  void SetEnableElementAPITypeCheckThrowWarning(bool enable) {
    enable_element_api_type_check_throw_warning_ = enable;
  }

  bool GetEnableElementAPITypeCheckThrowWarning() {
    return enable_element_api_type_check_throw_warning_;
  }

  void SetEnableBindICU(bool enable) { enable_bind_icu_ = enable; }

  bool GetEnableBindICU() { return enable_bind_icu_; }

  void SetEnableQueryComponentSync(bool enable) {
    enable_query_component_sync_ = enable;
  }

  bool GetEnableQueryComponentSync() const {
    return enable_query_component_sync_;
  }

  void SetDisableQuickTracingGC(bool disable) {
    disable_quick_tracing_gc_ = disable;
  }

  bool GetDisableQuickTracingGC() const { return disable_quick_tracing_gc_; }

  void SetFixCSSImportRuleOrder(bool enable) {
    fix_css_import_rule_order_ = enable;
  }

  bool GetFixCSSImportRuleOrder() const { return fix_css_import_rule_order_; }

  void SetEnableReloadLifecycle(bool enable) {
    enable_reload_lifecycle_ = enable;
  }

  bool GetEnableReloadLifecycle() { return enable_reload_lifecycle_; }

  void SetEnableOptPushStyleToBundle(TernaryBool enable) {
    enable_opt_push_style_to_bundle_ = enable;
  }

  TernaryBool GetEnableOptPushStyleToBundle() const {
    return enable_opt_push_style_to_bundle_;
  }

  inline void SetEnableFiberElementForRadonDiff(TernaryBool enable) {
    enable_fiber_element_for_radon_diff_ = enable;
  }

  inline TernaryBool GetEnableFiberElementForRadonDiff() const {
    return enable_fiber_element_for_radon_diff_;
  }

  inline void SetPreferredFps(const std::string& preferred_fps) {
    preferred_fps_ = preferred_fps;
  }

  inline std::string GetPreferredFps() { return preferred_fps_; }

  inline void SetEnableCSSLazyImport(TernaryBool enable_css_lazy_import) {
    enable_css_lazy_import_ = enable_css_lazy_import;
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

  inline void SetEnableNewAnimator(TernaryBool enable_new_animator) {
    enable_new_animator_ = enable_new_animator;
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

  TernaryBool GetEnableMicrotaskPromisePolyfill() const {
    return enable_microtask_promise_polyfill_;
  }

  void SetEnableMicrotaskPromisePolyfill(TernaryBool enable) {
    enable_microtask_promise_polyfill_ = enable;
  }

  TernaryBool GetEnableNativeScheduleCreateViewAsync() const {
    return enable_native_schedule_create_view_async_;
  }

  void SetEnableNativeScheduleCreateViewAsync(TernaryBool enable) {
    enable_native_schedule_create_view_async_ = enable;
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

  TernaryBool GetEnableSignalAPI() const { return enable_signal_api_; }

  bool GetEnableSignalAPIBoolValue() {
    if (enable_signal_api_ == TernaryBool::UNDEFINE_VALUE) {
      enable_signal_api_ = LynxEnv::GetInstance().EnableSignalAPI()
                               ? TernaryBool::TRUE_VALUE
                               : TernaryBool::UNDEFINE_VALUE;
    }
    return enable_signal_api_ == TernaryBool::TRUE_VALUE;
  }

  void SetEnableSignalAPI(TernaryBool enable) { enable_signal_api_ = enable; }

  // TODO(songshourui.null): move this function to testing file
  void PrintPageConfig(std::ostream& output) {
#define PAGE_CONFIG_DUMP(key) output << #key << ":" << key << ",";
    output << "page_version:" << page_version_ << ",";
    output << "page_flatten:" << page_flatten_ << ",";
    output << "page_implicit:" << page_implicit_ << ",";
    output << "dsl_:" << static_cast<int>(dsl_) << ",";
    PAGE_CONFIG_DUMP(enable_auto_show_hide)
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

 private:
  // Used for lynx config
  tasm::DynamicCSSConfigs css_configs_;
  // user defined extraInfo.
  lepus::Value extra_info_{};
  std::string target_sdk_version_;
  std::string lepus_version_;
  std::string absetting_disable_css_lazy_decode_;
  // peferredFps
  std::string preferred_fps_ = "auto";
  std::string original_config_{};
  // gc threshold of lepusNG. Let default value be 256, and the unit is KB.
  int64_t lepus_gc_threshold_{256};
  // force report lynx scroll fluency event.
  // When setting pageConfig.enableLynxScrollFluency to a double value in the
  // range [0, 1], we will monitor the fluency metrics for this LynxUI based on
  // this probability. The probability indicates the likelihood of enabling
  // fluency monitoring, and the metrics will be reported unconditionally
  // through the applogService.
  double enable_scroll_fluency_monitor{-1};
  // Composite config representing configs including enableParallelElement,
  // batch-rendering
  uint64_t pipeline_scheduler_config_{0};
  PackageInstanceBundleModuleMode bundle_module_mode_;
  // TernaryBool
  TernaryBool trail_New_Image_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool async_redirect_url{TernaryBool::UNDEFINE_VALUE};
  // Enable lazy_bundles to be decoded in child threads before they are
  // delivered into tasm in async-loading.
  TernaryBool enable_component_async_decode_{TernaryBool::UNDEFINE_VALUE};
  // enable use quick_context_pool to construct quick context
  TernaryBool enable_use_context_pool_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_use_map_buffer_{TernaryBool::UNDEFINE_VALUE};
  // introduced in 2.16, enable the optimization aboult UIOperation batching and
  // CreateViewAsync at Android
  TernaryBool enable_ui_operation_optimize_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_opt_push_style_to_bundle_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_fiber_element_for_radon_diff_{TernaryBool::UNDEFINE_VALUE};
  // Indicates whether use c++ list.
  TernaryBool enable_native_list_{TernaryBool::UNDEFINE_VALUE};
  // CSSLazyImport
  TernaryBool enable_css_lazy_import_{TernaryBool::UNDEFINE_VALUE};
  // enableNewAnimator
  TernaryBool enable_new_animator_{TernaryBool::UNDEFINE_VALUE};
  // enable microtask promise polyfill
  TernaryBool enable_microtask_promise_polyfill_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_native_schedule_create_view_async_{
      TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_signal_api_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_text_layout_cache_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_unified_pipeline_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_async_resolve_subtree_{TernaryBool::UNDEFINE_VALUE};
  // default include font padding
  // 1 means true
  // -1 means false
  int32_t include_font_padding_{0};
  int32_t observer_frame_rate_{20};
  int32_t long_press_duration_{-1};
  CSSParserConfigs css_parser_configs_;
  bool enable_a11y_mutation_observer{false};
  bool enable_a11y{false};
  PackageInstanceDSL dsl_;
  bool enable_auto_show_hide;
  bool enable_text_language_alignment_{false};
  bool enable_x_text_layout_reused_{false};
  // Default value is false. If this flag is true, the external gesture which's
  // state is possible or began will not cancel the Lynx iOS touch gesture see
  // issue:#7920.
  bool enable_touch_refactor_{true};
  // In the previous commit, when determining whether all fingers had moved off
  // the screen in multiple touch scenarios, touch.view was used for judgment.
  // However, in a scrolling container, touch.view obtained from
  // touchesEnd/touchesMove could be nil, resulting in incorrect judgment of
  // whether all fingers had moved off the screen. _touches could not be
  // cleared, leading to a subsequent failure to trigger tap events.
  // To fix this issue, we added null checks before calling touch.view, and
  // ended the gesture if _touches was empty. This resolved the problem.
  // only for ios, detail can see f-12375631 and its mr.
  bool enable_end_gesture_at_last_finger_up_{false};
  bool css_align_with_legacy_w3c_{false};
  bool enable_lepus_ng_{true};
  bool default_overflow_visible_{false};
  bool enable_save_page_data_{false};
  bool list_remove_component_{false};
  bool enable_z_index_{false};
  bool enable_remove_component_extra_data_{false};
  bool enable_lynx_air_{false};
  bool enable_fiber_arch_{false};
  bool enable_cascade_pseudo_{false};
  // Used for lynx config
  bool enable_css_parser_{false};
  bool is_target_sdk_verion_higher_than_2_1_{false};
  bool keyboard_callback_pass_relative_height_{false};
  bool enable_event_refactor_{true};
  bool force_calc_new_style_{true};
  bool enable_check_data_when_update_page_{true};
  bool compile_render_{false};
  // If this flag is true, iOS will not recognize the corresponding long press
  // gesture after triggering scrolling.
  bool disable_longpress_after_scroll_{false};

  bool enable_new_intersection_observer_{false};
  // The switch controlling whether to enable exposure detection optimization.
  bool enable_check_exposure_optimize_{false};
  // The switch controlling whether to enable send disexposure events when
  // lynxview is hidden.
  bool enable_disexposure_when_lynx_hidden_{true};
  // Enable exposure check when LynxView is layoutRequest. In certain scenarios,
  // exposure detection can be inaccurate if it is conducted before the layout
  // is complete. This is because the detection is calculated based on incorrect
  // positioning information. To address this issue, exposure detection is not
  // performed when LynxView isLayoutRequested. However, in some cases, LynxView
  // will call requestsLayout frequently, which prevents exposure detection from
  // being performed, resulting in fewer exposure events. To accommodate both
  // scenarios, a new enableExposureWhenLayout switch has been added to enable
  // businesses to control whether exposure detection is performed during
  // LynxView isLayoutRequested.
  bool enable_exposure_when_layout_{false};

  bool enable_exposure_ui_margin_{false};

  bool enable_new_gesture_{false};

  uint8_t map_container_type_{0};

  bool enable_check_local_image_{true};

  bool enable_async_request_image_{false};

  // Enable iOS background manager to apply shape layer optimization.
  bool enable_background_shape_layer_{true};

  CompileOptionAirMode air_mode_{CompileOptionAirMode::AIR_MODE_OFF};
  // set text overflow as visible if true

  // support component can be passed null props.
  // null props is only be supported in LepusNG now.
  // open this switch to support lepus use null prop.
  bool enable_component_null_prop_{false};

  // support CSS invalidation
  bool enable_css_invalidation_{false};

  // If false, descendant selector only works in component scope
  bool remove_descendant_selector_scope_{true};

  // indicate that enable standard css selector
  bool enable_standard_css_selector_{false};

  // indicate that enable data processor on js thread.
  bool enable_data_processor_on_js_{false};

  // enable support multi-finger events
  bool enable_multi_touch_{false};

  // enable support multi-finger event parameter compatibility.
  bool enable_multi_touch_params_compatible_{false};

  // enable harmony to detect exposure with visible area change event.
  bool enable_harmony_visible_area_change_for_exposure_{false};

  // enable air mode to detect removed keys in updating data from native
  bool enable_air_detect_removed_keys_when_update_data_{false};

  // Indicates whether the parallel flush of Element has been enabled. And the
  // default value is false.
  bool enable_parallel_element_{false};

  // enable raster animation
  bool enable_raster_animation_{false};

  // enable js binding api throw exception rather than report
  bool enable_js_binding_api_throw_exception_{false};

  // enable avoid throwing RenderFatal for element api when argument type
  // checking failed
  bool enable_element_api_type_check_throw_warning_{false};

  // enable LynxUI onNodeReload lifecycle;
  bool enable_reload_lifecycle_{false};

  // enable bind primjs-icu
  bool enable_bind_icu_{false};

  bool enable_query_component_sync_{false};

  // disable tracing gc mode in quick context
  bool disable_quick_tracing_gc_{false};

  bool fix_css_import_rule_order_{true};

  /**
   * Not a config but a marker to indicate whether the page config needs to be
   * posted to platform layer. In PreDecode, PageConfig will be set to platform
   * layer before `LoadBundle`, so that it does not need to be posted to
   * platform layer again, which can reduce the overhead of posting. This
   * optimization is only valid for Android now.
   * TODO(zhoupeng.z): Apply this optimization to all platforms.
   */
  bool need_post_to_platform_{true};

  template <typename T>
  using PageConfigSetter = void (PageConfig::*)(T);

  template <typename T>
  using PageConfigGetter = T (PageConfig::*)() const;

  template <typename T>
  using PageConfigPair = std::pair<PageConfigSetter<T>, PageConfigGetter<T>>;

  template <typename T>
  using PageConfigMap = std::unordered_map<std::string, PageConfigPair<T>>;

  static const PageConfigMap<TernaryBool>& GetFuncBoolMap();

  static const PageConfigMap<uint64_t>& GetFuncUint64Map();
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PAGE_CONFIG_H_
